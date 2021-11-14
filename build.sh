#! /bin/bash

# Make a directory in /tmp/openfpm_data
#

workspace=$1
hostname=$(hostname)
branch=$3


echo "Directory: $workspace"
echo "Machine: $hostname"
echo "Branch: $branch"


mkdir /tmp/openfpm_vcluster
mv * .[^.]* /tmp/openfpm_vcluster
mv /tmp/openfpm_vcluster openfpm_vcluster

mkdir openfpm_vcluster/src/config

## It is equivalent to the git modules

git clone git@git.mpi-cbg.de:/openfpm/openfpm_devices.git openfpm_devices
git clone git@git.mpi-cbg.de:/openfpm/openfpm_data.git openfpm_data
cd openfpm_data
git checkout master
cd ..
cd openfpm_devices
git checkout master
cd ..

cd "$workspace/openfpm_vcluster"

# install MPI and BOOST  LIBHILBERT if needed

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/LIBHILBERT ]; then
        ./install_LIBHILBERT.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4
fi


if [ x"$hostname" == x"cifarm-centos-node.mpi-cbg.de"  ]; then
        echo "CentOS node"
        source /opt/rh/devtoolset-7/enable
	export PATH="$HOME/openfpm_dependencies/openfpm_vcluster/CMAKE/bin:$PATH"
fi

if [ x"$hostname" == x"cifarm-ubuntu-node"  ]; then
        echo "Ubuntu node"
        export PATH="/opt/bin:$PATH"
fi

if [ x"$hostname" == x"cifarm-mac-node" ]; then
	export PATH="$HOME/openfpm_dependencies/openfpm_vcluster/CMAKE/bin:$PATH"
fi

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/BOOST ]; then
        if [ x"$hostname" == x"cifarm-mac-node" ]; then
                echo "Compiling for OSX"
                ./install_BOOST.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4 darwin
        else
                echo "Compiling for Linux"
                ./install_BOOST.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4 gcc
        fi
fi

./install_CMAKE_on_CI.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/MPI ]; then
	./install_MPI.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4
fi

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/LIBHILBERT ]; then
        ./install_LIBHILBERT.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4
fi

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/VCDEVEL ]; then
        ./install_VCDEVEL.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4 gcc g++
fi

export PATH="$PATH:$HOME/openfpm_dependencies/openfpm_vcluster/MPI/bin"

source $HOME/.bashrc
echo "$PATH"
echo "Compiling general"
sh ./autogen.sh
if [ x"$hostname" == x"cifarm-mac-node" ]; then
	./configure  CXX=mpic++ --with-vcdevel=$HOME/openfpm_dependencies/openfpm_vcluster/VCDEVEL  --with-boost=$HOME/openfpm_dependencies/openfpm_vcluster/BOOST --with-libhilbert=$HOME/openfpm_dependencies/openfpm_vcluster/LIBHILBERT  --enable-cuda-on-cpu
else
	./configure  CXX=mpic++ --with-vcdevel=$HOME/openfpm_dependencies/openfpm_vcluster/VCDEVEL  --with-boost=$HOME/openfpm_dependencies/openfpm_vcluster/BOOST --with-libhilbert=$HOME/openfpm_dependencies/openfpm_vcluster/LIBHILBERT  --with-cuda-on-backend=OpenMP
fi
make VERBOSE=1 -j 4
if [ $? -ne 0 ]; then exit 1 ; fi

