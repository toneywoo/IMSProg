#!/bin/bash

# IMSProg 依赖安装脚本
# 用法: sudo ./install_dependencies.sh

set -e

echo "========================================="
echo "  IMSProg 依赖安装脚本"
echo "========================================="
echo ""

# 检测系统类型
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo "❌ 无法检测操作系统类型"
    exit 1
fi

echo "检测到操作系统: $OS $VERSION"
echo ""

# 根据系统类型安装依赖
case $OS in
    ubuntu|debian|linuxmint|pop)
        echo "📦 正在安装 Debian/Ubuntu 依赖..."
        apt update
        
        # 基础编译工具
        apt install -y cmake g++ pkg-config
        
        # libusb
        apt install -y libusb-1.0-0-dev
        
        # Qt5 相关
        apt install -y qtbase5-dev qttools5-dev
        
        # systemd/udev
        if [ "$(echo "$VERSION >= 23.10" | bc)" -eq 1 ] 2>/dev/null || \
           [ "$(echo "$VERSION >= 13" | bc)" -eq 1 ] 2>/dev/null; then
            apt install -y systemd-dev
        else
            apt install -y udev
        fi
        
        echo "✅ Debian/Ubuntu 依赖安装完成"
        ;;
    
    fedora|rhel|centos)
        echo "📦 正在安装 Fedora/RHEL 依赖..."
        dnf install -y \
            cmake \
            gcc-c++ \
            pkgconf-pkg-config \
            libusb1-devel \
            qt5-qtbase-devel \
            qt5-linguist
        echo "✅ Fedora/RHEL 依赖安装完成"
        ;;
    
    arch|manjaro)
        echo "📦 正在安装 Arch Linux 依赖..."
        pacman -Sy --noconfirm \
            cmake \
            gcc \
            pkgconf \
            libusb \
            qt5-base \
            qt5-tools
        echo "✅ Arch Linux 依赖安装完成"
        ;;
    
    opensuse*)
        echo "📦 正在安装 openSUSE 依赖..."
        zypper install -y \
            cmake \
            gcc-c++ \
            pkgconfig \
            libusb-1_0-devel \
            libqt5-qtbase-devel \
            libqt5-linguist-devel
        echo "✅ openSUSE 依赖安装完成"
        ;;
    
    *)
        echo "❌ 不支持的操作系统: $OS"
        echo "请手动安装以下依赖:"
        echo "  - cmake"
        echo "  - g++ (或 gcc-c++)"
        echo "  - libusb-1.0 开发库"
        echo "  - Qt5 开发库 (qtbase, qttools)"
        echo "  - pkg-config"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "  ✅ 所有依赖安装完成！"
echo "========================================="
echo ""
echo "现在可以运行编译脚本:"
echo "  ./build_only.sh      # 仅编译（无需 root）"
echo "  sudo ./build_all.sh  # 编译并安装到系统"
echo ""
