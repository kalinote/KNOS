#!/bin/bash

# 函数：检查操作系统是否为 CentOS 7
check_os() {
    if [ -f /etc/os-release ]; then
        # 加载操作系统信息
        . /etc/os-release
        if [ "$NAME" = "CentOS Linux" ] && [ "$VERSION_ID" = "7" ]; then
            echo "Confirmed: CentOS 7"
        else
            echo "Error: This script can only be run on CentOS 7"
            exit 1
        fi
    else
        echo "Error: Cannot determine the operating system"
        exit 1
    fi
}

# 主执行逻辑
check_os

# 在这里加入脚本的其余部分
yum check-update
yum install nasm gcc ruby as ld make python3 -y

# bochs config
# ./configure --with-x11 --with-wx --enable-debugger --enable-disasm --enable-all-optimizations --enable-readline --enable-long-phy-address --enable-ltdl-install --enable-idle-hack --enable-plugins --enable-a20-pin --enable-x86-64 --enable-smp --enable-cpu-level=6 --enable-large-ramfile --enable-repeat-speedups --enable-fast-function-calls  --enable-handlers-chaining  --enable-trace-linking --enable-configurable-msrs --enable-show-ips --enable-cpp --enable-debugger-gui --enable-iodebug --enable-logging --enable-assert-checks --enable-fpu --enable-vmx=2 --enable-svm --enable-3dnow --enable-alignment-check  --enable-monitor-mwait --enable-avx  --enable-evex --enable-x86-debugger --enable-pci --enable-usb --enable-voodoo
# yum install libXrandr-devel
