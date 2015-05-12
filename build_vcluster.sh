#! /bin/bash

# Make a directory in /tmp/OpenFPM_data

mkdir /tmp/openfpm_vcluster
mv * .[^.]* /tmp/openfpm_vcluster
mv /tmp/openfpm_vcluster OpenFPM_vcluster

mkdir OpenFPM_vcluster/src/config

git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_devices.git OpenFPM_devices
git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_data.git OpenFPM_data

cd "$1/OpenFPM_vcluster"

sh ./autogen.sh
sh ./configure CXX=mpic++
make

mpirun -np 2 ./src/vcluster
mpirun -np 4 ./src/vcluster
mpirun -np 16 ./src/vcluster
