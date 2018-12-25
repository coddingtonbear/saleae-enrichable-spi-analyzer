set -x

mkdir build
cd build

# Install a modern version of cmake
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py --user
bin/pip install --user cmake

# Build
bin/cmake ..
make
