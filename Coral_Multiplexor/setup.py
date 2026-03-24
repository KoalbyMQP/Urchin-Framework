from setuptools import find_packages, setup


setup(
    name="coral_multiplexor",
    version="0.1.0",
    packages=find_packages(),
    include_package_data=True,
    install_requires=["setuptools"],
    zip_safe=True,
    entry_points={
        "console_scripts": [
            "coral_multiplexor_node = coral_multiplexor.node:main",
        ],
    },
)
