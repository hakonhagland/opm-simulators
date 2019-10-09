# Python wrappers for the OPM flow simulator

- This repository is a fork
of [opm-simulators](https://github.com/OPM/opm-simulators) for testing out python wrappers for the flow simulator.
- OPM flow simulator is written in C++, see
[the official README.md](https://github.com/OPM/opm-simulators/blob/master/README.md) for
information about the OPM project. 
- The Python-to-C++ interface is written using
  the [pybind11](https://github.com/pybind/pybind11) C++ library.
## Installation

### Install libecl

Install libecl, see [Installing libecl](https://opm-project.org/?page_id=239) at the
OPM project page. For example,

```
git clone https://github.com/Statoil/libecl.git
cd libecl
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/libecl
make
sudo make install
# NOTE: you may want to add this statement to you ~/.bashrc file:
export ecl_DIR=/opt/libecl/share/cmake/ecl
```

### Install DUNE modules

Install DUNE core modules (`dune-common`, `dune-geometry`,
`dune-istl`, `dune-grid`), see
the 
[OPM prerequisites page](https://opm-project.org/?page_id=239). For
example, for Ubuntu:

```
# Make sure we have updated URLs to packages etc.
sudo apt-get update -y

# For server edition of Ubuntu add-apt-repository depends on
sudo apt-get install -y software-properties-common

# Add PPA for OPM packages
sudo add-apt-repository -y ppa:opm/ppa
sudo apt-get update -y

# Packages necessary for building
sudo apt-get install -y build-essential gfortran pkg-config cmake

# Packages necessary for documentation
sudo apt-get install -y doxygen ghostscript texlive-latex-recommended pgf gnuplot

# Packages necessary for version control
sudo apt-get install -y git-core

# MPI for parallel programs
sudo apt-get install -y mpi-default-dev
# Prerequisite libraries
sudo apt-get install -y libblas-dev libboost-all-dev \
  libsuperlu-dev libsuitesparse-dev libtrilinos-zoltan-dev

# Parts of Dune needed
sudo apt-get install libdune-common-dev libdune-geometry-dev \
  libdune-istl-dev libdune-grid-dev

```
### Install OPM modules (including this repository)

```
# NOTE: remember to set the ecl_DIR environment variable as described above,
#   variable before proceeding
#
mkdir opm
cd opm
export OPM_INSTALL_DIR="$PWD"
# clone this repository
git clone https://github.com/hakonhagland/opm-simulators
# Get the installation script
cp opm-simulators/python/bin/install_and_build_opm_modules.sh .
# Run the script to clone and build all the OPM modules
./install_and_build_opm_modules.sh
```

# Testing the installation

```
SUNBEAM_PATH=$OPM_INSTALL_DIR/opm-common/build/python
SIMULATORS_PATH=$OPM_INSTALL_DIR/opm-simulators/build/python
export PYTHONPATH=${PYTHONPATH}:${SUNBEAM_PATH}:${SIMULATORS_PATH}
cd $OPM_INSTALL_DIR/opm-simulators/python/test_imp
python3 run.py
```

# Compiling

If you make changes to the code in `opm-simulators`, you can recompile
the Python `simulators` module by doing:
```
cd $OPM_INSTALL_DIR/opm-simulators/build
make -j$(nproc)
```


