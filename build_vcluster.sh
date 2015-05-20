#! /bin/bash

# Make a directory in /tmp/OpenFPM_data

echo "Directory: $1"
echo "Machine: $2"

mkdir /tmp/openfpm_vcluster
mv * .[^.]* /tmp/openfpm_vcluster
mv /tmp/openfpm_vcluster OpenFPM_vcluster

mkdir OpenFPM_vcluster/src/config

git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_devices.git OpenFPM_devices
git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_data.git OpenFPM_data

cd "$1/OpenFPM_vcluster"

echo "Compiling on $2"

if [ "$2" == "gin" ]
then
 echo "Compiling on gin\n"
 module load gcc/4.9.2
 module load openmpi/1.8.1
fi

if [ "$2" == "wetcluster" ]
then
 echo "Compiling on wetcluster\n"
 module load gcc/4.9.2
 module load openmpi/1.8.1
 module load boost/1.54.0
 COMPILE_OPTIONS = '--with-boost="/sw/apps/boost/1.54.0/"'
fi

sh ./autogen.sh
sh ./configure $(COMPILE_OPTIONS)  CXX=mpic++
make


sh ./autogen.sh
sh ./configure CXX=mpic++
make

if [ $2 == "wetcluster" ]
then
 bsub -K -n 2 mpirun -np 2 ./src/vcluster
 if [ "$?" = "0" ]; then exit 1 ; fi
else
 echo "VCLUSTER\n"
# mpirun -np 2 ./src/vcluster
# mpirun -np 4 ./src/vcluster
fi

