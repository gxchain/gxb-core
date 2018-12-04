#!/bin/bash
ARCH=$(uname)
if [ "$ARCH" == "Linux" ]; then
    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
    if [ "$OS_NAME" != "Ubuntu" ]; then
        printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
        exit 1
    fi
    curl -L 'https://github.com/gxchain/gxb-core/releases/download/testnet-1.0.181130/gxb_ubuntu_1.0.181130.testnet.tar.gz' -o gxb_ubuntu_1.0.181126.testnet.tar.gz
    tar zxvf gxb_ubuntu_1.0.181126.testnet.tar.gz
elif [ "$ARCH" == "Darwin" ]; then
    curl -L 'https://github.com/gxchain/gxb-core/releases/download/testnet-1.0.181126/gxb_osx_1.0.181126.testnet.tar.gz' -o gxb_osx_1.0.181126.testnet.tar.gz
    tar zxvf gxb_osx_1.0.181126.testnet.tar.gz
else
    printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
    exit 1
fi
