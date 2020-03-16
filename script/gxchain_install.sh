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
    if [ "$OS_VERSION" == "14.04" ]; then
        curl -L 'https://github.com/gxchain/gxb-core/releases/download/v1.0.200327/gxb_1.0.200327-ubuntu-14.04.tar.gz' -o gxb_1.0.200327-ubuntu-14.04.tar.gz 
        tar zxvf gxb_1.0.200327-ubuntu-14.04.tar.gz
    else
        curl -L 'https://github.com/gxchain/gxb-core/releases/download/v1.0.200327/gxb_1.0.200327-ubuntu-14.04.tar.gz' -o gxb_1.0.200327-ubuntu-14.04.tar.gz 
        tar zxvf gxb_1.0.200327-ubuntu-14.04.tar.gz
    fi
elif [ "$ARCH" == "Darwin" ]; then
    curl -L 'https://github.com/gxchain/gxb-core/releases/download/v1.0.190318/gxb_1.0.190318-osx.tar.gz' -o gxb_1.0.190318-osx.tar.gz
    tar zxvf gxb_1.0.190318-osx.tar.gz
else
    printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
    exit 1
fi
