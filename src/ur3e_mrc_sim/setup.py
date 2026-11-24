import os
from glob import glob
from setuptools import find_packages, setup

package_name = 'ur3e_mrc_sim'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob(os.path.join('launch', '*launch.[pxy][yma]*'))),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Ivan Penskiy',
    maintainer_email='ipenskiy@umd.edu',
    description='TODO: Package description',
    license='BSD-3-Clause',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            "ur3e_mrc_enpm662_ctrl = ur3e_mrc_sim.ur3e_mrc_enpm662_ctrl:main",
            "ur3e_mrc_enpm662_topics = ur3e_mrc_sim.ur3e_mrc_enpm662_topics:main",
        ],
    },
)
