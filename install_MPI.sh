#!/bin/bash 

mkdir ${HOME}/MPI
wget http://www.open-mpi.de/software/ompi/v1.8/downloads/openmpi-1.8.5.tar.bz2
tar -xvf openmpi-1.8.5.tar.bz2
cd openmpi-1.8.5
sh ./configure --prefix=${HOME}/MPI --enable-opal-multi-threads --enable-mpi-f90
make
make install
