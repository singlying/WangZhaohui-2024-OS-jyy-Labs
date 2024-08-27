NAME := test
SRCS := test.c
export MODULE  := L0
export AM_HOME := $(PWD)/../abstract-machine

# 检车ARCH是否为空
ifeq ($(ARCH),)
# ARCH通常用于指定目标体系结构  x86_64-qemu，表示构建目标为x86_64架构并使用QEMU进行仿真。
export ARCH := x86_64-qemu
# 对应上面的if
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git
