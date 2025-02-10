#!/bin/bash
# compile.sh

# 输出颜色设置
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "Starting compilation..."

# 编译服务器
echo -n "Compiling server... "
if cc server.c -o server -lpthread -std=c99 -D_POSIX_C_SOURCE=200809L; then
    echo -e "${GREEN}Success${NC}"
else
    echo -e "${RED}Failed${NC}"
    exit 1
fi

# 编译客户端
echo -n "Compiling client... "
if cc client.c -o client -lpthread -lncurses -std=c99; then
    echo -e "${GREEN}Success${NC}"
else
    echo -e "${RED}Failed${NC}"
    exit 1
fi

echo "Compilation completed successfully!"

# 显示编译后的文件信息
echo -e "\nCompiled files:"
ls -l server client