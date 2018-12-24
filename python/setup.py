from setuptools import setup, find_packages
from os import path

here = path.abspath(path.dirname(__file__))
with open(path.join(here, '../readme.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name="saleae_enrichable_spi_analyzer",
    version="1.0",
    description=(
        "Easily generate custom markers and bubble text for arbitrary "
        "SPI protocols"
    ),
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/coddingtonbear/saleae-enrichable-spi-analyzer",
    author="Adam Coddington",
    author_email="me@adamcoddington.net",
    classifiers=[
        'Development Status :: 4 - Beta',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
    ],
    packages=find_packages()
)
