#!/bin/bash

# IMSProg 构建脚本 - 仅编译，不安装（不需要 root 权限）
# 用法: ./build_only.sh

set -e  # 遇到错误立即退出

echo "========================================="
echo "  IMSProg 编译脚本（无需 root 权限）"
echo "========================================="
echo ""

# 检查必要的工具
for cmd in cmake make g++; do
    if ! command -v $cmd &> /dev/null; then
        echo "❌ 错误: 未找到 $cmd，请先安装开发工具"
        exit 1
    fi
done

echo "✅ 开发工具检查通过"
echo ""

# 编译函数
build_module() {
    local module=$1
    local module_name=$2
    
    echo "-----------------------------------------"
    echo "正在编译: $module_name"
    echo "-----------------------------------------"
    
    if [ ! -d "$module" ]; then
        echo "❌ 错误: 目录 $module 不存在"
        return 1
    fi
    
    cd "$module"
    
    # 清理旧的构建目录
    if [ -d "build" ]; then
        echo "清理旧的构建目录..."
        rm -rf build/
    fi
    
    # 创建并进入构建目录
    mkdir build
    cd build
    
    # 配置 CMake
    echo "配置 CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # 编译
    echo "开始编译（使用 $(nproc) 个核心）..."
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo "✅ $module_name 编译成功！"
        echo "   可执行文件位置: $(pwd)/$module_name"
        
        # 显示文件大小
        if [ -f "$module_name" ]; then
            local size=$(du -h "$module_name" | cut -f1)
            echo "   文件大小: $size"
        fi
    else
        echo "❌ $module_name 编译失败"
        return 1
    fi
    
    cd ../..
    echo ""
}

# 编译所有模块
build_module "IMSProg_programmer" "IMSProg (主程序)"
build_module "IMSProg_editor" "IMSProg Editor (数据库编辑器)"
build_module "IMSProg_database_update" "IMSProg Database Update (数据库更新工具)"

echo "========================================="
echo "  ✅ 所有模块编译完成！"
echo "========================================="
echo ""
echo "📁 编译产物位置:"
echo "   主程序:     IMSProg_programmer/build/IMSProg"
echo "   编辑器:     IMSProg_editor/build/IMSProg_editor"
echo "   更新工具:   IMSProg_database_update/build/IMSProg_database_update"
echo ""
echo "🚀 运行测试（无需安装）:"
echo "   cd IMSProg_programmer/build && ./IMSProg"
echo ""
echo "💡 提示:"
echo "   - 如果测试正常，可以使用 'sudo ./build_all.sh' 安装到系统"
echo "   - 或者手动复制到 ~/bin 目录: cp IMSProg_programmer/build/IMSProg ~/bin/"
echo "   - 需要 udev 规则才能以普通用户访问 CH341A 设备"
echo ""
