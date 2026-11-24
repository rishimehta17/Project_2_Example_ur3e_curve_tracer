#include "VacuumGripper.hpp"

/// Adopted from https://github.com/osrf/mbzirc/blob/main/mbzirc_ign/src/SuctionGripper.cc

using namespace mrc_gazebo_plugins;
using namespace ignition;
using namespace gazebo;

class mrc_gazebo_plugins::VacuumGripperPrivate
{
  /// \brief The item being moved
  public: Entity childItem{kNullEntity};

  /// \brief The gripper link name
  public: std::string linkName;

  /// \brief Used to store the joint when we attach to an object
  public: Entity joint{kNullEntity};

  /// \brief The gripper link entity
  public: Entity gripperEntity{kNullEntity};

  /// \brief The transport node
  public: transport::Node node;

  /// \brief Used for determining when the suction is on.
  public: bool suctionOn{false};

  /// \brief Set to true when we detect the suction gripper is in contact
  public: bool pendingJointCreation{false};

  /// \brief True when we are holding an object
  public: bool jointCreated{false};

  /// \brief mutex for accessing member variables
  public: std::mutex mtx;

  /// \brief Array of contact points
  public: std::array<Entity, 1> contacts;

  /// \brief Publisher for contact points
  public: transport::Node::Publisher contactPublisher;

  /// \brief Callback for when contact is made
  public: void OnContact(int idx0,
              const ignition::msgs::Contacts &_msg)
  {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (_msg.contact_size())
    {
      auto contact = _msg.contact(0);
      this->contacts[idx0] = contact.collision2().id();
    }
    else
    {
      this->contacts[idx0] = kNullEntity;
    }
  }

  /// \brief Command callback
  public: void OnCmd(const ignition::msgs::Boolean &_suctionOn)
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->suctionOn = _suctionOn.data();
  }
};

//////////////////////////////////////////////////
VacuumGripperPlugin::VacuumGripperPlugin():
  dataPtr(new VacuumGripperPrivate)
{
  for (size_t ii = 0; ii < 1; ++ii)
  {
    this->dataPtr->contacts[ii] = kNullEntity;
  }
}

//////////////////////////////////////////////////
VacuumGripperPlugin::~VacuumGripperPlugin()
{
}

//////////////////////////////////////////////////
void VacuumGripperPlugin::Configure(const Entity &_entity,
  const std::shared_ptr<const sdf::Element> &_sdf,
  EntityComponentManager &_ecm,
  EventManager &/*_eventMgr*/)
{
  if(_sdf->HasElement("parent_link"))
  {
    this->dataPtr->linkName = _sdf->Get<std::string>("parent_link");
  }
  else
  {
    ignerr << "Please specify a link name" << std::endl;
    return;
  }

  Model model(_entity);
  this->dataPtr->gripperEntity = model.LinkByName(_ecm, this->dataPtr->linkName);
  if (this->dataPtr->gripperEntity == kNullEntity)
  {
    ignerr << "Could not find link named "
      << this->dataPtr->linkName << std::endl;
    return;
  }

  if(_sdf->HasElement("contact_sensor_topic_prefix"))
  {
    auto prefix = _sdf->Get<std::string>("contact_sensor_topic_prefix");

    std::function<void(const ignition::msgs::Contacts &)> callback_0 =
      std::bind(&VacuumGripperPrivate::OnContact, this->dataPtr.get(), 0,
        std::placeholders::_1);
    this->dataPtr->node.Subscribe(prefix + "/contact_sensor_0", callback_0);

    this->dataPtr->contactPublisher =
      this->dataPtr->node.Advertise<msgs::Boolean>(prefix + "/contacts/suction_cup");
  }
  else
  {
    ignerr << "Please specify a contact_sensor_topic_prefix" << std::endl;
    return;
  }

  if(_sdf->HasElement("command_topic"))
  {
    auto topic = _sdf->Get<std::string>("command_topic");
    this->dataPtr->node.Subscribe(
      topic,
      &VacuumGripperPrivate::OnCmd,
      this->dataPtr.get());
  }
  else
  {
    ignerr << "Please specify a command_topic" << std::endl;
    return;
  }
}


//////////////////////////////////////////////////
void VacuumGripperPlugin::PreUpdate(const UpdateInfo &_info,
  EntityComponentManager &_ecm)
{
  if (_info.paused) return;
  std::lock_guard<std::mutex> lock(this->dataPtr->mtx);

  msgs::Boolean contact;

  // If the gripper is engaged and holding an object, return contacts as true
  if (this->dataPtr->jointCreated)
  {
    contact.set_data(true);
    this->dataPtr->contactPublisher.Publish(contact);
  }
  else
  {
    contact.set_data(this->dataPtr->contacts[0] != kNullEntity);
    this->dataPtr->contactPublisher.Publish(contact);
  }

  if (!this->dataPtr->jointCreated && this->dataPtr->suctionOn)
  {
    bool contactMade = (this->dataPtr->contacts[0] != kNullEntity);
    if (contactMade)
    {
      this->dataPtr->pendingJointCreation = true;
      this->dataPtr->childItem = this->dataPtr->contacts[0];
    }
    else
    {
      ignwarn << "Make a contact before issuing vacuum command" << std::endl;
      this->dataPtr->pendingJointCreation = false;
    }
  }

  // Clear contacts
  for (size_t ii = 0; ii < 1; ++ii)
  {
    this->dataPtr->contacts[ii] = kNullEntity;
  }

  if (this->dataPtr->pendingJointCreation)
  {
    // If we need to create a new joint
    this->dataPtr->pendingJointCreation = false;
    this->dataPtr->joint = _ecm.CreateEntity();
    auto parentLink = _ecm.ParentEntity(this->dataPtr->childItem);
    _ecm.CreateComponent(
          this->dataPtr->joint,
          components::DetachableJoint({this->dataPtr->gripperEntity,
                                       parentLink, "fixed"}));
    igndbg << "Created joint between gripper and "
      << this->dataPtr->childItem
      << std::endl << "at time step " << _info.simTime.count() << std::endl;
    this->dataPtr->jointCreated = true;
  }

  if (!this->dataPtr->suctionOn && this->dataPtr->jointCreated)
  {
    // If we have an item and were commanded to release it
    _ecm.RequestRemoveEntity(this->dataPtr->joint);
    this->dataPtr->joint = kNullEntity;
    this->dataPtr->jointCreated = false;
    igndbg << "Remove joint between gripper and "
      << this->dataPtr->childItem
      << std::endl << "at time step " << _info.simTime.count() << std::endl;
  }
}


IGNITION_ADD_PLUGIN(
    mrc_gazebo_plugins::VacuumGripperPlugin,
    ignition::gazebo::System,
    mrc_gazebo_plugins::VacuumGripperPlugin::ISystemConfigure,
    mrc_gazebo_plugins::VacuumGripperPlugin::ISystemPreUpdate)