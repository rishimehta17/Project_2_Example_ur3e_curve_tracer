from setuptools import setup

package_name = 'ur3e_curve_tracer'

setup(
    name=package_name,
    version='0.1.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    entry_points={
        'console_scripts': [
            'trace_curve = ur3e_curve_tracer.trace_curve:main',
            'live_plot = ur3e_curve_tracer.live_plot:main',
        ],
    },
)
