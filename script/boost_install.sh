#!/bin/bash
# install boost 1.57.0 on MacOS / linux

sys_os=`uname -s`
if [ $sys_os == "Darwin" ]; then
	echo "----- MacOS -----"
elif [ $sys_os == "Linux" ]; then
	echo "----- Linux -----"
else
	echo "only support MaxOS / Linux"
    exit 1
fi

# download boost 1.57.0
wget 'http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.gz' -O boost_1_57_0.tar.gz
tar zxvf boost_1_57_0.tar.gz

cd boost_1_57_0
bash ./bootstrap.sh --prefix=/usr

if [ $sys_os == "Darwin" ]; then
    ./b2 --buildtype=complewte install toolset=clang cxxflags="-arch x86_64" linkflags="-arch x86_64"
else 
    if [ $(id -u) != 0 ]; then
        SUDO="sudo"
    fi
    $SUDO ./b2 --buildtype=complete install
fi


