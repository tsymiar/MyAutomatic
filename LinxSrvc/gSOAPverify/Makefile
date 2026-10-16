# gSOAPverify 构建
#   make            : 编译，产物 ../bin/gSOAPverify（并同步 myweb.wsdl）
#   make clean      : 只删本项目自己的 .o 与可执行文件，不动 ../bin 下其它项目
#   make install    : 拷贝可执行文件到 $(PREFIX)（默认 /usr/sbin），需 root
CUR  := $(shell pwd)
GEN  := $(CUR)/../bin
OBJ  := $(CUR)/../out/gSOAPverify
WSDL := ./soap/myweb.wsdl
TGT  := $(GEN)/gSOAPverify

CXX  ?= g++
# -DNS_DBG 打开调试日志（含明文口令与完整 SQL）。上生产时覆盖它即可:
#   make FLAG="-O2 -std=c++11"
FLAG ?= -Wall -g -DNS_DBG -std=c++11
PTHD := -lpthread

# MySQL 头文件/库：优先 pkg-config，其次 mysql_config
MYSQL_CFLAGS := $(shell pkg-config --cflags mysqlclient 2>/dev/null \
                  || mysql_config --cflags 2>/dev/null)
MYSQL_LIBS   := $(shell pkg-config --libs mysqlclient 2>/dev/null \
                  || mysql_config --libs 2>/dev/null)
HAVE_MYSQL   := $(if $(MYSQL_LIBS),yes,$(shell test -f /usr/include/mysql/mysql.h && echo yes))

SRCS := soap/soapC.cpp soap/stdsoap2.cpp soap/soapServer.cpp \
        sys/status.cc  sql/sqlDbReq.cc  main/mainSoap.cc
OBJS := $(addprefix $(OBJ)/,$(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(SRCS))))

PREFIX ?= /usr/sbin

.PHONY: all gSOAPverify check-mysql clean clear install

all: gSOAPverify

gSOAPverify: check-mysql $(TGT)

# 没装 MySQL 开发库时必须真的失败，不能只打印成功
check-mysql:
ifeq ($(HAVE_MYSQL),)
	@echo "\033[31mERROR\033[0m: MySQL/MariaDB 客户端开发库未找到，无法构建。" >&2; \
	 echo "  Debian/Ubuntu: sudo apt install libmysql++-dev" >&2; \
	 echo "  RHEL/CentOS  : sudo yum install mariadb-devel" >&2; \
	 exit 1
else
	@true
endif

$(TGT): $(OBJS)
	@mkdir -p $(GEN)
	$(CXX) $(FLAG) -o $@ $(OBJS) $(MYSQL_LIBS) $(PTHD)
	@cp -f $(WSDL) $(GEN)/
	@echo "-- Built: \033[1m$@\033[0m"

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FLAG) -I.. $(MYSQL_CFLAGS) -c $< -o $@

$(OBJ)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(FLAG) -I.. $(MYSQL_CFLAGS) -c $< -o $@

# 只清理本项目产物：../out/gSOAPverify 与 ../bin/gSOAPverify
clean:
	@rm -rf $(OBJ)
	@rm -f $(TGT)

clear: clean

install: gSOAPverify
	install -m 0755 $(TGT) $(PREFIX)/gSOAPverify
	@echo "-- installed: $(PREFIX)/gSOAPverify"
