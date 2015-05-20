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

if [ "$2" == "gin" ]
then
 echo "Compiling on gin\n"
 module load gcc/4.9.2
 module load openmpi/1.8.1
fi

#if [ $2 == "wetcluster" ]
#then
# bsub -K -n 2 mpirun -np 2 ./src/vcluster
# if [ "$?" = "0" ]; then exit 1 ; fi
#else
# echo "VCLUSTER\n"
# mpirun -np 2 ./src/vcluster
# mpirun -np 4 ./src/vcluster
#fi

if [ "$2" == "wetcluster" ]
then
 echo "Compiling on wetcluster"

## produce the module path

export MODULEPATH="/sw/apps/modules/modulefiles:$MODULEPATH"

 script="module load gcc/4.9.2\n 
module load openmpi/1.8.1\n
module load boost/1.54.0\n
compile_options='--with-boost=/sw/apps/boost/1.54.0/'\n
\n
sh ./autogen.sh\n
sh ./configure \"\$compile_options\"  CXX=mpic++\n
make\n
if [ \"\$?\" = "0" ]; then exit 1 ; fi\n
exit(0)\n"

 echo $script | sed -r 's/\\n/\n/g' > compile_script

 bsub -o output.%J -K -n 1 -J compile sh ./compile_script

## Run on the cluster
 bsub -o output.%J -K -n 2 mpirun -np 2 ./src/vcluster
fi


