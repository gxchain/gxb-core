	export LANG=en_US.UTF-8
	TEMP_DIR=/tmp
	BOOST_ROOT=${HOME}/opt/boost
	OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' \
	| cut -d'.' -f1 )

	MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )
	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu -pCPU | grep -v "#" | wc -l )
	MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
	JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

	DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
	DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
	DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
	DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
	DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))

	printf "\\n\\tOS name: %s\\n" "${OS_NAME}"
	printf "\\tOS Version: %s\\n" "${OS_VER}"
	printf "\\tCPU speed: %sMhz\\n" "${CPU_SPEED}"
	printf "\\tCPU cores: %s\\n" "${CPU_CORE}"
	printf "\\tPhysical Memory: %s Mgb\\n" "${MEM_MEG}"
	printf "\\tDisk install: %s\\n" "${DISK_INSTALL}"
	printf "\\tDisk space total: %sG\\n" "${DISK_TOTAL%.*}"
	printf "\\tDisk space available: %sG\\n" "${DISK_AVAIL%.*}"
	printf "\\tConcurrent Jobs (make -j): ${JOBS}\\n"

	if [ "${OS_VER}" -lt 7 ]; then
		printf "\\n\\tYou must be running Centos 7 or higher to install GXChain.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
		printf "\\n\\tYou must have at least %sGB of available storage to install GXChain.\\n" "${DISK_MIN}"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	printf "\\n"

        printf "\\tChecking Yum installation...\\n"
        if ! YUM=$( command -v yum 2>/dev/null ); then
                printf "\\t!! Yum must be installed to compile EOS.IO !!\\n"
                printf "\\tExiting now.\\n"
                exit 1;
        fi
        printf "\\t- Yum installation found at %s.\\n" "${YUM}"

        printf "\\tUpdating YUM repository...\\n"
        if ! sudo "${YUM}" -y update > /dev/null 2>&1; then
                printf "\\t!! YUM update failed !!\\n"
                printf "\\tExiting now.\\n"
                exit 1;
        fi
        printf "\\t - YUM repository successfully updated.\\n"

	printf "\\tChecking installation of Centos Software Collections Repository...\\n"
	SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' )
	if [ -z "${SCL}" ]; then
		printf "\\t - Do you wish to install and enable this repository?\\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* )
					printf "\\tInstalling SCL...\\n"
					if ! sudo "${YUM}" -y --enablerepo=extras install centos-release-scl 2>/dev/null; then
						printf "\\t!! Centos Software Collections Repository installation failed !!\\n"
						printf "\\tExiting now.\\n\\n"
						exit 1;
					else
						printf "\\tCentos Software Collections Repository installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "\\tUser aborting installation of required Centos Software Collections Repository, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else
		printf "\\t - ${SCL} found.\\n"
	fi

        printf "\\tChecking installation of devtoolset-7...\\n"
        DEVTOOLSET=$( rpm -qa | grep -E 'devtoolset-7-[0-9].*' )
        if [ -z "${DEVTOOLSET}" ]; then
                printf "\\tDo you wish to install devtoolset-7?\\n"
                select yn in "Yes" "No"; do
                        case $yn in
                                [Yy]* )
                                        printf "\\tInstalling devtoolset-7...\\n"
                                        if ! sudo "${YUM}" install -y devtoolset-7 2>/dev/null; then
                                                printf "\\t!! Centos devtoolset-7 installation failed !!\\n"
                                                printf "\\tExiting now.\\n"
                                                exit 1;
                                        else
                                                printf "\\tCentos devtoolset installed successfully.\\n"
                                        fi
                                break;;
                                [Nn]* ) echo "User aborting installation of devtoolset-7. Exiting now."; exit;;
                                * ) echo "Please type 1 for yes or 2 for no.";;
                        esac
                done
        else
                printf "\\t - ${DEVTOOLSET} found.\\n"
        fi
	printf "\\tEnabling Centos devtoolset-7...\\n"
	if ! source "/opt/rh/devtoolset-7/enable" 2>/dev/null; then
		printf "\\t!! Unable to enable Centos devtoolset-7 at this time !!\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	printf "\\tCentos devtoolset-7 successfully enabled.\\n"

        printf "\\tChecking installation of python33...\\n"
        PYTHON33=$( rpm -qa | grep -E 'python33-[0-9].*' )
        if [ -z "${PYTHON33}" ]; then
                printf "\\tDo you wish to install python33?\\n"
                select yn in "Yes" "No"; do
                        case $yn in
                                [Yy]* )
                                        printf "\\tInstalling Python33...\\n"
                                        if ! sudo "${YUM}" install -y python33.x86_64 2>/dev/null; then
                                                printf "\\t!! Centos Python33 installation failed !!\\n"
                                                printf "\\tExiting now.\\n"
                                                exit 1;
                                        else
                                                printf "\\n\\tCentos Python33 installed successfully.\\n"
                                        fi

                                break;;
                                [Nn]* ) echo "User aborting installation of python33. Exiting now."; exit;;
                                * ) echo "Please type 1 for yes or 2 for no.";;
                        esac
                done
        else
                printf "\\t - ${PYTHON33} found.\\n"
        fi

	printf "\\n"

	DEP_ARRAY=( git autoconf automake bzip2 libtool ocaml.x86_64 doxygen graphviz-devel.x86_64 \
	libicu-devel.x86_64 bzip2.x86_64 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 \
	python-devel.x86_64 gettext-devel.x86_64)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\\tChecking YUM for installed dependencies.\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( sudo "${YUM}" info "${DEP_ARRAY[$i]}" 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )
		if [ "$pkg" != "installed" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n\\t"
			printf "\\t!! Package %s ${bldred} NOT ${txtrst} found !!\\n" "${DEP_ARRAY[$i]}"
			(( COUNT++ ))
		else
			printf "\\t - Package %s found.\\n" "${DEP_ARRAY[$i]}"
			continue
		fi
	done

	printf "\\n"

	if [ "${COUNT}" -gt 1 ]; then
		printf "\\tThe following dependencies are required to install GXChain.\\n"
		printf "\\t${DISPLAY}\\n\\n"
		printf "\\tDo you wish to install these dependencies?\\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* )
					printf "\\tInstalling dependencies\\n\\n"
					if ! sudo "${YUM}" -y install ${DEP}; then
						printf "\\t!! YUM dependency installation failed !!\\n"
						printf "\\tExiting now.\\n"
						exit 1;
					else
						printf "\\tYUM dependencies installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else
		printf "\\t - No required YUM dependencies to install.\\n"
	fi

	printf "\\n"

	if [ "${ENABLE_COVERAGE_TESTING}" = true ]; then
		printf "\\tChecking perl installation...\\n"
		perl_bin=$( command -v perl 2>/dev/null )
		if [ -z "${perl_bin}" ]; then
			printf "\\tInstalling perl...\\n"
			if ! sudo "${YUM}" -y install perl; then
				printf "\\t!! Unable to install perl at this time !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		else
			printf "\\t - Perl installation found at %s.\\n" "${perl_bin}"
		fi

		printf "\\n"

		printf "\\tChecking LCOV installation...\\n"
		lcov=$( command -v lcov 2>/dev/null )
		if [ -z  "${lcov}" ]; then
			printf "\\tInstalling LCOV...\\n"
			if ! cd "${TEMP_DIR}"; then
				printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			LCOVURL="https://github.com/linux-test-project/lcov.git"
			if ! git clone "${LCOVURL}"; then
				printf "\\t!! Unable to clone LCOV from ${LCOVURL} !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! cd "${TEMP_DIR}/lcov"; then
				printf "\\t!! Unable to enter directory %s/lcov !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! sudo make install; then
				printf "\\t!! Unable to install LCOV at this time !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! cd "${CWD}"; then
				printf "\\t!! Unable to enter directory %s !!\\n" "${CWD}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! rm -rf "${TEMP_DIR}/lcov"; then
				printf "\\t!! Unable to remove directory %s/lcov !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			printf "\\tSuccessfully installed LCOV.\\n"
		else
			printf "\\t - LCOV installation found @ %s.\\n" "${lcov}"
		fi
	fi

	printf "\\n"

	printf "\\tReinstall CMAKE3.11...\\n"
	rm -rf /opt/cmake /tmp/cmake-3.11.0-Linux-x86_64.sh /usr/local/bin/cmake
    cd /tmp && wget https://cmake.org/files/v3.11/cmake-3.11.0-Linux-x86_64.sh
    mkdir -p /opt/cmake && chmod +x /tmp/cmake-3.11.0-Linux-x86_64.sh
    bash /tmp/cmake-3.11.0-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
    ln -sfT /opt/cmake/bin/cmake /usr/local/bin/cmake

	BOOSTTGZ="boost_1_67_0.tar.bz2"
	BOOSTFOLDER=$(echo "${BOOSTTGZ}" | sed 's/.tar.bz2//g')
	if [ -d "${HOME}/opt/${BOOSTFOLDER}" ]; then
		if ! mv "${HOME}/opt/${BOOSTFOLDER}" "${BOOST_ROOT}"; then
			printf "\\t!! Unable to move directory %s/opt/${BOOSTFOLDER} to %s !!\\n" "${HOME}" "${BOOST_ROOT}"
			printf "\\tExiting now.\\n"
			exit 1
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"; then
				printf "\\t!! Unable to remove directory %s: Please delete it and try again !! 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		fi
	fi

	printf "\\tChecking boost library installation...\\n"
	BOOSTVERSION=$( grep "#define BOOST_VERSION" "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null \
	| tail -1 | tr -s ' ' | cut -d\  -f3)
	if [ "${BOOSTVERSION}" != "106700" ]; then
		printf "\\tRemoving existing boost libraries in %s/opt/boost*...\\n" "${HOME}"
		if ! rm -rf "${HOME}"/opt/boost*; then
			printf "\\t!! Unable to remove deprecated boost libraries at %s/opt/boost* !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tInstalling boost libraries...\\n"
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		BOOSTURL="https://dl.bintray.com/boostorg/release/1.67.0/source/${BOOSTTGZ}"
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 "${BOOSTURL}")
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\t!! Unable to download Boost libraries from ${BOOSTURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\textracting boost code...\\n"
		if ! tar xf "${TEMP_DIR}/${BOOSTTGZ}"; then
			printf "\\t!! Unable to unarchive file %s/${BOOSTTGZ} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f  "${TEMP_DIR}/${BOOSTTGZ}"; then
			printf "\\t!! Unable to remove file %s/${BOOSTTGZ} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/${BOOSTFOLDER}/"; then
			printf "\\t!! Unable to enter directory %s/${BOOSTFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! ./bootstrap.sh --prefix=$BOOST_ROOT; then
			printf "\\t!! Installation of boost libraries failed with the above error !! 0\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! $TEMP_DIR/$BOOSTFOLDER/b2 -j"${JOBS}" install; then
			printf "\\t!! Installation of boost libraries in ${BOOST_ROOT} failed with the above error !! 1\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/${BOOSTFOLDER}/"; then
			printf "\\t!! Unable to remove directory %s/boost_1_67_0 !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tBoost successfully installed @ %s.\\n" "${BOOST_ROOT}"
	else
		printf "\\t - Boost ${BOOSTVERSION} found at %s.\\n" "${BOOST_ROOT}"
	fi

	printf "\\n"

	printf "\\tChecking LLVM with WASM support installation...\\n"
	if [ ! -d "${HOME}/opt/wasm/bin" ]; then
		printf "\\tInstalling LLVM with WASM...\\n"
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler"  2>/dev/null; then
			printf "\\t!! Unable to create directory %s/llvm-compiler !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler"; then
			printf "\\t!! Unable to enter directory %s/llvm-compiler !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMURL="https://github.com/llvm-mirror/llvm.git"
		if ! git clone --depth 1 --single-branch --branch release_40 "${LLVMURL}"; then
			printf "\\t!! Unable to clone llvm repo from ${LLVMURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMLOCATION="llvm-compiler/llvm/tools"
		if ! cd "${TEMP_DIR}/${LLVMLOCATION}"; then
			printf "\\t!! Unable to enter directory %s/${LLVMLOCATION} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		CLANGURL="https://github.com/llvm-mirror/clang.git"
		if ! git clone --depth 1 --single-branch --branch release_40 "${CLANGURL}"; then
			printf "\\t!! Unable to clone clang repo from ${CLANGURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMMIDLOCATION=$(echo $LLVMLOCATION | sed 's/\/tools//g')
		if ! cd "${TEMP_DIR}/${LLVMMIDLOCATION}"; then
			printf "\\t!! Unable to enter directory %s/${LLVMMIDLOCATION} !!\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/${LLVMMIDLOCATION}/build" 2>/dev/null; then
			printf "\\t!! Unable to create directory %s/${LLVMMIDLOCATION}/build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/${LLVMMIDLOCATION}/build"; then
			printf "\\t!! Unable to enter directory %s/${LLVMMIDLOCATION}/build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${HOME}/opt/wasm" \
		-DLLVM_TARGETS_TO_BUILD="host" -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="WebAssembly" \
		-DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE="Release" ..; then
			printf "\\t!! CMake has exited with the above error !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"; then
			printf "\\t!! Compiling LLVM with EXPERIMENTAL WASM support has exited with the above errors !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make install; then
			printf "\\t!! Installing LLVM with EXPERIMENTAL WASM support has exited with the above errors !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMFOLDER=$(echo $LLVMMIDLOCATION | | sed 's/\/llvm//g')
		if ! rm -rf "${TEMP_DIR}/${LLVMFOLDER}" 2>/dev/null; then
			printf "\\t!! Unable to remove directory %s/${LLVMFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tWASM compiler successfully installed at %s/opt/wasm\\n" "${HOME}"
	else
		printf "\\t - WASM found at %s/opt/wasm\\n" "${HOME}"
	fi

	printf "\\n"
