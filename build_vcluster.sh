#! /bin/bash

# Make a directory in /tmp/openfpm_data

echo "$PATH"
echo "Directory: $1"
echo "Machine: $2"

mkdir /tmp/openfpm_vcluster
mv * .[^.]* /tmp/openfpm_vcluster
mv /tmp/openfpm_vcluster openfpm_vcluster

mkdir openfpm_vcluster/src/config

git clone git@ppmcore.mpi-cbg.de:/incardon/openfpm_devices.git openfpm_devices
git clone git@ppmcore.mpi-cbg.de:/incardon/openfpm_data.git openfpm_data

cd "$1/openfpm_vcluster"

if [ "$2" == "gin" ]; then
 echo "Compiling on gin\n"
 module load gcc/4.9.2
 module load openmpi/1.8.1
elif [ "$2" == "wetcluster" ]; then
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
 bsub -o output_run2.%J -K -n 2 -R "span[hosts=1]" "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 2 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run4.%J -K -n 4 -R "span[hosts=1]" "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 4 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run8.%J -K -n 8 -R "span[hosts=1]" "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 8 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
 bsub -o output_run12.%J -K -n 12 -R "span[hosts=1]" "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 12 ./src/vcluster"
 if [ $? -ne 0 ]; then exit 1 ; fi
# bsub -o output_run32.%J -K -n 32 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 32 ./src/vcluster"
# if [ $? -ne 0 ]; then exit 1 ; fi
# bsub -o output_run32.%J -K -n 64 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 64 ./src/vcluster"
# if [ $? -ne 0 ]; then exit 1 ; fi
# bsub -o output_run32.%J -K -n 128 "module load openmpi/1.8.1 ; module load gcc/4.9.2;  mpirun -np 128 ./src/vcluster"
# if [ $? -ne 0 ]; then exit 1 ; fi
elif [ "$2" == "taurus" ]; then
 echo "Compiling on taurus"

 echo "$PATH"
 module load gcc/5.3.0
 module load boost/1.60.0
 module load openmpi/1.10.2-gnu
 module unload bullxmpi

 sh ./autogen.sh
 sh ./configure --with-boost=/sw/taurus/libraries/boost/1.55.0-gnu4.8  CXX=mpic++
 make
 if [ $? -ne 0 ]; then exit 1 ; fi

### to exclude --exclude=taurusi[6300-6400],taurusi[5400-5500]

 salloc --nodes=1 --ntasks-per-node=24 --time=00:05:00 --mem-per-cpu=1800 --partition=haswell bash -c "ulimit -s unlimited && mpirun -np 24 src/vcluster --report_level=no"
 if [ $? -ne 0 ]; then exit 1 ; fi
 sleep 5
 salloc --nodes=2 --ntasks-per-node=24 --time=00:05:00 --mem-per-cpu=1800 --partition=haswell bash -c "ulimit -s unlimited && mpirun -np 48 src/vcluster --report_level=no"
 if [ $? -ne 0 ]; then exit 1 ; fi
 sleep 5
 salloc --nodes=4 --ntasks-per-node=24 --time=00:05:00 --mem-per-cpu=1800 --partition=haswell bash -c "ulimit -s unlimited && mpirun -np 96 src/vcluster --report_level=no"
 if [ $? -ne 0 ]; then exit 1 ; fi
 sleep 5
 salloc --nodes=8 --ntasks-per-node=24 --time=00:05:00 --mem-per-cpu=1800 --partition=haswell bash -c "ulimit -s unlimited && mpirun -np 192 src/vcluster --report_level=no"
 if [ $? -ne 0 ]; then exit 1 ; fi
 sleep 5
 salloc --nodes=10 --ntasks-per-node=24 --time=00:5:00 --mem-per-cpu=1800 --partition=haswell bash -c "ulimit -s unlimited && mpirun -np 240 src/vcluster --report_level=no"
 if [ $? -ne 0 ]; then exit 1 ; fi

else

 source $HOME/.bashrc
 echo "$PATH"
 echo "Compiling general"
 sh ./autogen.sh
 sh ./configure  CXX=mpic++
 make

 mpirun -np 2 ./src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 mpirun -np 3 ./src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
 mpirun -np 4 ./src/vcluster
 if [ $? -ne 0 ]; then exit 1 ; fi
fi

curl -X POST --data "payload={\"icon_emoji\": \":jenkins:\", \"username\": \"jenkins\"  , \"attachments\":[{ \"title\":\"Info:\", \"color\": \"#00FF00\", \"text\":\"$2 completed succeffuly the openfpm_vcluster test \" }] }" https://hooks.slack.com/services/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
