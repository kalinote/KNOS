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
yum install nasm gcc ruby as ld make -y
