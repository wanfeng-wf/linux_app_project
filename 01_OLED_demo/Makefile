REMOTE_HOST = root@192.168.7.2
TARGET_PATH = $(REMOTE_HOST):/root/oled

CROSS_COMPILE ?= aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

INC_DIR = ./inc
SRC_DIR = ./src
BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/main

CFLAGS = -Wall -I$(INC_DIR)

# ./src/*.c
SRCS = $(shell find $(SRC_DIR) -name '*.c')
# ./src/*.c => ./build/*.o
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
# ./src/*.c => ./build/*.d
DEPS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.d, $(SRCS))

# 默认目标:
all: $(TARGET)

# -MMD: 生成依赖文件，但忽略系统头文件
# -MP:  为头文件生成伪目标，防止删除头文件导致 make 报错
# -MF:  指定生成的 .d 文件名
# -MT:  指定 .d 文件中目标的名字（确保带上 build/ 路径）
DEPFLAGS = -MMD -MP -MF $(@:.o=.d) -MT $@

# 编译的同时生成依赖
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# 链接:
$(TARGET): $(OBJS)
	@echo "buiding $@..."
	@mkdir -p $(dir $@)
	$(CC) -o $(TARGET) $(OBJS)

push:
	scp $(TARGET) $(TARGET_PATH)

.PHONY: all clean push
# 清理 build 目录:
clean:
	@echo "clean..."
	rm -rf $(BUILD_DIR)

# 引入所有 .d 文件:
-include $(DEPS)
