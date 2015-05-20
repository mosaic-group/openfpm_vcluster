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
elif [ "$2" == "wetcluster" ]
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

 bsub -o output_compile.%J -K -n 1 -J compile sh ./compile_script

## Run on the cluster
 bsub -o output_run2.%J -K -n 2 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 2 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run4.%J -K -n 4 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 4 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run8.%J -K -n 8 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 8 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
else
 echo "Compiling general"
 sh ./autogen.sh
 sh ./configure  CXX=mpic++
 make

 mpirun -np 2 ./src/vcluster
 mpirun -np 4 ./src/vcluster

fi


