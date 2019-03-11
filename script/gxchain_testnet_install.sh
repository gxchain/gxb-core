#!/bin/bash
set -x

ARCH=$(uname)
if [ "$ARCH" == "Linux" ]; then
    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
    if [ "$OS_NAME" != "Ubuntu" ]; then
        printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
        exit 1
    fi
    OS_VERSION=$(lsb_release -a | grep "Release" | sed -e 's/Release:[\t]*//g')
    echo "Your OS Version: Ubuntu ${OS_VERSION}"
    if [ "$OS_VERSION" = "14.04" ]; then
        curl -L 'https://github.com/gxchain/gxb-core/releases/download/testnet-1.0.190311/gxb_1.0.190311-ubuntu-14.04.testnet.tar.gz' -o gxb_1.0.190311-ubuntu-14.04.testnet.tar.gz
        tar zxvf gxb_1.0.190311-ubuntu-14.04.testnet.tar.gz
    else
        curl -L 'https://github.com/gxchain/gxb-core/releases/download/testnet-1.0.190311/gxb_1.0.190311-ubuntu-14.04.testnet.tar.gz' -o gxb_1.0.190311-ubuntu-16.04.testnet.tar.gz
        tar zxvf gxb_1.0.190311-ubuntu-16.04.testnet.tar.gz
    fi
elif [ "$ARCH" == "Darwin" ]; then
    curl -L 'https://github.com/gxchain/gxb-core/releases/download/testnet-1.0.190311/gxb_1.0.190311-osx.testnet.tar.gz' -o gxb_1.0.190311-osx.testnet.tar.gz
    tar zxvf gxb_1.0.190311-osx.testnet.tar.gz
else
    printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
    exit 1
fi