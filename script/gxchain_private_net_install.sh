#!/bin/bash
ARCH=$(uname)
if [ "$ARCH" == "Linux" ]; then
    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
    if [ "$OS_NAME" != "Ubuntu" ]; then
        printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
        exit 1
    fi
    curl -L 'http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/private-net/gxb_gxb_ubuntu_1.0.180929.private-net.tar.gz' -o gxb_ubuntu_private-net.tar.gz
    tar zxvf gxb_ubuntu_private-net.tar.gz
elif [ "$ARCH" == "Darwin" ]; then
    curl -L 'http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/private-net/gxb_osx_1.0.180929.private-net.tar.gz' -o gxb_osx_private-net.tar.gz
    tar zxvf gxb_osx_private-net.tar.gz
else
    printf "\\n\\tGXChain core currently supports macOS & Ubuntu Linux only.\\n"
    exit 1
fi
