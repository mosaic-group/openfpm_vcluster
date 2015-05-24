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
 bsub -o output_run16.%J -K -n 16 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 16 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run32.%J -K -n 32 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 32 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run32.%J -K -n 64 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 64 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run32.%J -K -n 128 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 128 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
elif [ "$2" == "taurus" ]
then
 echo "Compiling on taurus"

 echo "$PATH"
 module load gcc/4.8.2
 module load boost/1.55.0-gnu4.8
 module unload bullxmpi
 module load openmpi/1.8.5

 sh ./autogen.sh
 sh ./configure  CXX=mpic++
 make
 if [ $? -ne 0 ]; then exit 1 ; fi

 salloc --nodes=1 --ntasks-per-node=16 --time=04:00:00 --mem-per-cpu=1900 --partition=sandy mpirun -np 16 src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 salloc --nodes=2 --ntasks-per-node=16 --time=04:00:00 --mem-per-cpu=1900 --partition=sandy mpirun -np 32 src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 salloc --nodes=4 --ntasks-per-node=16 --time=04:00:00 --mem-per-cpu=1900 --partition=sandy mpirun -np 64 src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 salloc --nodes=8 --ntasks-per-node=16 --time=04:00:00 --mem-per-cpu=1900 --partition=sandy mpirun -np 128 src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 salloc --nodes=16 --ntasks-per-node=16 --time=04:00:00 --mem-per-cpu=1900 --partition=sandy mpirun -np 256 src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi

else
 echo "Compiling general"
 sh ./autogen.sh
 sh ./configure  CXX=mpic++
 make

 mpirun -np 2 ./src/vcluster
 mpirun -np 4 ./src/vcluster

fi


