# Makefile for LS-IQCQP

CXX := g++
CXXFLAGS ?= -O3
LDFLAGS ?= -static
LDLIBS ?= -lgsl -lgslcblas -lm

TARGET := LS-IQCQP
SRCS := ls_read.cpp ls_no_cons.cpp ls_bin.cpp component.cpp ls_mix_not_dis.cpp ls_balance.cpp gradient.cpp call.cpp

.PHONY: all clean run help

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(SRCS) $(LDFLAGS) $(CXXFLAGS) -o $(TARGET) $(LDLIBS)

.PHONY: debug
debug: CXXFLAGS += -DGRADIENT_DEBUG -g
debug: $(TARGET)

# 用法: make run ARGS="cutoff filename"
run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	$(RM) $(TARGET) *.o

help:
	@echo "make            # 编译生成 $(TARGET)"
	@echo "make run ARGS=\"cutoff filename\"  # 运行程序，传入截止时间与文件名"
	@echo "make clean      # 清理生成物"


