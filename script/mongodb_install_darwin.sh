#!/bin/bash
# install mongodb and mongodb driver(mongo-c-driver-1.10.2/mongo-cxx-driver-v3.3) on MacOS / linux

# install mongodb
printf "\\tInstall MongoDB Server and MongoDB Driver\\n"
printf "\\tChecking MongoDB Server \\n"
printf "\\tDo you want to install MongoDB Server?\\n"
select yn in "Yes" "No"; do
	case $yn in
		[Yy]* ) 
			printf "\\n\\n\\tInstalling MongoDB.\\n\\n"
			wget https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-3.6.3.tgz
			tar -zxvf mongodb-osx-ssl-x86_64-3.6.3.tgz
			sudo mv mongodb-osx-x86_64-3.6.3 /usr/local/mongodb
			if ! rm -f "mongodb-osx-ssl-x86_64-3.6.3.tgz"
				then
					printf "\\tUnable to remove file mmongodb-osx-ssl-x86_64-3.6.3.tgz.\\n"
					printf "\\tExiting now.\\n\\n"
					exit 1;
			fi
		break;;
		[Nn]* ) break;;
		* ) break;;
	esac
done


# install mongdb driver
printf "\\n\\tChecking MongoDB driver installation.\\n"
INSTALL_DRIVER=true

# checking c++ driver version
if [ -e "/usr/local/lib/libmongocxx-static.a" ]; then
	INSTALL_DRIVER=false
	if ! version=$( grep "Version:" /usr/local/lib/pkgconfig/libmongocxx-static.pc | tr -s ' ' | awk '{print $2}' )
	then
		printf "\\tUnable to determine mongodb-cxx-driver version.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	maj=$( echo "${version}" | cut -d'.' -f1 )
	min=$( echo "${version}" | cut -d'.' -f2 )
	if [ "${maj}" -gt 3 ]; then
		INSTALL_DRIVER=true
	elif [ "${maj}" -eq 3 ] && [ "${min}" -lt 3 ]; then
		INSTALL_DRIVER=true
	fi
fi
INSTALL_DRIVER=true
# install driver
if [ $INSTALL_DRIVER == "true" ]; then

	# install mongodb c driver
	STATUS=$( curl -LO -w '%{http_code}' --connect-timeout 30 https://github.com/mongodb/mongo-c-driver/releases/download/1.10.2/mongo-c-driver-1.10.2.tar.gz )
	if [ "${STATUS}" -ne 200 ]; then
		if ! rm -f "mongo-c-driver-1.10.2.tar.gz"
		then
			printf "\\tUnable to remove file mongo-c-driver-1.10.2.tar.gz.\\n" 
		fi
		printf "\\tUnable to download MongoDB C driver at this time.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! tar xf mongo-c-driver-1.10.2.tar.gz
	then
		printf "\\tUnable to unarchive file mongo-c-driver-1.10.2.tar.gz.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! rm -f "mongo-c-driver-1.10.2.tar.gz"
	then
		printf "\\tUnable to remove file mongo-c-driver-1.10.2.tar.gz.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! cd mongo-c-driver-1.10.2
	then
		printf "\\tUnable to cd into directory mongo-c-driver-1.10.2.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! mkdir cmake-build
	then
		printf "\\tUnable to create directory mongo-c-driver-1.10.2/cmake-build.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! cd cmake-build
	then
		printf "\\tUnable to enter directory mongo-c-driver-1.10.2/cmake-build.\\n" 
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON \
	-DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON ..
	then
		printf "\\tConfiguring MongoDB C driver has encountered the errors above.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	CPU_CORE=$( sysctl -in machdep.cpu.core_count )
	if ! make -j"${CPU_CORE}"
	then
		printf "\\tError compiling MongoDB C driver.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! sudo make install
	then
		printf "\\tError installing MongoDB C driver.\\nMake sure you have sudo privileges.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	cd ../../
	if ! rm -rf "mongo-c-driver-1.10.2"
	then
		printf "\\tUnable to remove directory mongo-c-driver-1.10.2.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	# install mongodb c++ driver
	if ! git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/v3.3 --depth 1
	then
		printf "\\tUnable to clone MongoDB C++ driver at this time.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! cd "mongo-cxx-driver/build"
	then
		printf "\\tUnable to enter directory mongo-cxx-driver/build.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
	then
		printf "\\tCmake has encountered the above errors building the MongoDB C++ driver.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! make -j"${CPU_CORE}"
	then
		printf "\\tError compiling MongoDB C++ driver.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	if ! sudo make install
	then
		printf "\\tError installing MongoDB C++ driver.\\nMake sure you have sudo privileges.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	cd ../../
	if ! rm -rf "mongo-cxx-driver"
	then
		printf "\\tUnable to remove directory mongo-cxx-driver.\\n" 
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	printf "\\tMongo C++ driver installed at /usr/local/lib/libmongocxx-static.a.\\n"
else
	printf "\\tMongo C++ driver found at /usr/local/lib/libmongocxx-static.a.\\n"
fi

