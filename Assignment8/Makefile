# 顶层 Makefile
# 用法：
#   make        —— 自动完成 CMake 配置 + 编译（代码改动时增量编译，无需手动 cmake）
#   make run    —— 编译并运行
#   make clean  —— 删除 build 目录
# 原理：cmake 无配置变化时几乎零开销；真正的增量编译由 build/Makefile 负责。

BUILD_DIR := build

.PHONY: all run clean

all:
	cmake -B $(BUILD_DIR) -S . >/dev/null
	$(MAKE) -C $(BUILD_DIR) -j$(shell nproc)

run: all
	./$(BUILD_DIR)/ropesim

clean:
	rm -rf $(BUILD_DIR)
