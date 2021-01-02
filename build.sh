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

#if [ x"$hostname" == x"cifarm-mac-node" ]; then
#	echo "Killing ghost"
#	kill 73440 87662 87661 73439 51687 51686
#fi

rm -rf $HOME/openfpm_dependencies/openfpm_vcluster/BOOST

if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/BOOST ]; then
        if [ x"$hostname" == x"cifarm-mac-node" ]; then
                echo "Compiling for OSX"
                ./install_BOOST.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4 darwin
        else
                echo "Compiling for Linux"
                ./install_BOOST.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4 gcc
        fi
fi


if [ ! -d $HOME/openfpm_dependencies/openfpm_vcluster/MPI ]; then
	./install_MPI.sh $HOME/openfpm_dependencies/openfpm_vcluster/ 4
fi

export PATH="$PATH:$HOME/openfpm_dependencies/openfpm_vcluster/MPI/bin"

if [ "$hostname" == "gin" ]; then
 echo "Compiling on gin\n"
 module load gcc/4.9.2
 module load openmpi/1.8.1

elif [ "$hostname" == "wetcluster" ]; then
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

elif [ "$hostname" == "taurus" ]; then
 echo "Compiling on taurus"

 echo "$PATH"
 module load gcc/5.3.0
 module load boost/1.60.0
 module load openmpi/1.10.2-gnu
 module unload bullxmpi

 sh ./autogen.sh
 sh ./configure  CXX=mpic++
 make
 if [ $? -ne 0 ]; then exit 1 ; fi

### to exclude --exclude=taurusi[6300-6400],taurusi[5400-5500]

else

 source $HOME/.bashrc
 echo "$PATH"
 echo "Compiling general"
 sh ./autogen.sh
 sh ./configure  CXX=mpic++ --with-boost=$HOME/openfpm_dependencies/openfpm_vcluster/BOOST
 make VERBOSE=1 -j 4
 if [ $? -ne 0 ]; then exit 1 ; fi

fi

