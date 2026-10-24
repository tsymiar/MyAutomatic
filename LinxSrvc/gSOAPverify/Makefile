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
MYSQL_CFLAGS ?= $(shell pkg-config --cflags mysqlclient 2>/dev/null \
                  || mysql_config --cflags 2>/dev/null)
MYSQL_LIBS   ?= $(shell pkg-config --libs mysqlclient 2>/dev/null \
                  || mysql_config --libs 2>/dev/null)
HAVE_MYSQL := $(shell echo 'int main(){mysql_library_init(0,0,0);return 0;}' \
                | $(CXX) $(MYSQL_CFLAGS) -include mysql/mysql.h -x c++ - -o /dev/null \
                  $(MYSQL_LIBS) >/dev/null 2>&1 && echo yes)

SRCS := soap/soapC.cpp soap/stdsoap2.cpp soap/soapServer.cpp \
        sys/status.cc  main/mainSoap.cc
ifeq ($(HAVE_MYSQL),yes)
FLAG += -DHAVE_MYSQL
SRCS += sql/sqlDbReq.cc
endif
OBJS := $(addprefix $(OBJ)/,$(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(SRCS))))

PREFIX ?= /usr/sbin

.PHONY: all gSOAPverify check-mysql fix clean clear install

all: gSOAPverify

gSOAPverify: check-mysql $(TGT)

# 提示是否构建 MySQL
check-mysql:
ifeq ($(HAVE_MYSQL),yes)
	@echo "-- MySQL: \033[32menabled\033[0m"
else
	@echo "-- MySQL/MariaDB client development libraries not found, skipping mysql code."
	@echo "   To enable:  Debian/Ubuntu: sudo apt install libmysql++-dev"
	@echo "               RHEL/CentOS  : sudo yum install mariadb-devel"
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

# Windows 侧编译的 fix/soapClient_fix.cpp 由 soap/soapClient.cpp 派生，编译前生成
fix:
	@python3 soap/gen_client_fix.py

# 清理产物：../out/gSOAPverify 与 ../bin/gSOAPverify
clean:
	@rm -rf $(OBJ)
	@rm -f $(TGT)

clear: clean

install: gSOAPverify
	install -m 0755 $(TGT) $(PREFIX)/gSOAPverify
	@echo "-- installed: $(PREFIX)/gSOAPverify"
