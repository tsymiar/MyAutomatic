# gSOAPverify Introduce

基于 **gSOAP 2.8.106** 的多线程 SOAP 服务端（RPC/encoded），提供 `trans`、
`get-server-status`、`login-by-key` 三个接口，后者查询本机 MySQL 的 `myautomatic.glkline` 表。

> 实测环境：aarch64 / Ubuntu 22.04 / g++ 11.4。文中结论均来自真实编译与真实请求，非静态推断。

---

## 1. 运行状态

| 能力 | 状态 | 实测 |
|---|---|---|
| 编译链接 | ✅ | 6 个源文件 + `-lmysqlclient -lpthread`，产出 ~890KB ELF |
| `GET /?wsdl` | ✅ | HTTP 200（要求 CWD 下有 `myweb.wsdl`） |
| `trans` | ✅ | `Param(0): usr[tom]` |
| `get-server-status` | ✅ | `req=1000` → `7.0067`（空闲内存百分比） |
| `login-by-key` | ✅ | `flag`：200 命中 / -2 查询失败 / -3 参数缺失 |

⚠️ 演示与内网可用；上生产前请先看第 8 节「遗留问题」。

## 2. 目录

```
gSOAPverify/
├── Makefile                  # 构建（产物 ../bin/gSOAPverify）
├── main/mainSoap.cc/.h       # 【手写】main、线程池+环形队列、http_get、3 个接口实现
├── soap/
│   ├── rpcapi.h              # 【手写·关键】gSOAP 的真正输入头文件
│   ├── soapC.cpp / soapH.h / soapStub.h / soapServer.cpp   # 【生成】序列化 + 分派
│   ├── stdsoap2.cpp/.h       # 【引擎，已内嵌源码】
│   ├── myweb.wsdl / api.xsd / myweb.nsmap                  # 契约（ns = urn:myweb）
│   ├── myweb.*.{req,res}.xml # 6 个示例报文
│   └── make.sh               # 重新生成代码
├── sql/sqlDbReq.cc/.h        # 【手写】MySQL 连接、重连看门狗、查询
├── sys/status.cc/.h          # 【手写】内存/CPU 统计
├── file/myautomatic.sql      # 建库建表 + 2 条样例数据
└── fix/                      # 遗留补丁，不参与编译
```

**参与编译的只有 6 个文件**：`soap/{soapC,stdsoap2,soapServer}.cpp`、
`sys/status.cc`、`sql/sqlDbReq.cc`、`main/mainSoap.cc`。

## 3. 依赖

| 依赖 | 必需 | 说明 |
|---|---|---|
| g++（C++11） | ✅ | 实测 11.4 |
| MySQL/MariaDB 客户端库 | ✅ | `<mysql/mysql.h>` + `-lmysqlclient`；Debian `libmysql++-dev`，RHEL `mariadb-devel` |
| pthread | ✅ | `-lpthread` |
| MySQL 服务端 | ⚠️ | 仅 `login-by-key` 需要 |
| OpenSSL / zlib | ❌ | 未启用 → **只有明文 HTTP** |

## 4. 编译

```bash
cd LinxSrvc/gSOAPverify
D=/tmp/gsv_build && mkdir -p $D && F="-Wall -g -DNS_DBG -std=c++11"
g++ $F -c soap/soapC.cpp      -o $D/soapC.o
g++ $F -c soap/soapServer.cpp -o $D/soapServer.o
g++ $F -c soap/stdsoap2.cpp   -o $D/stdsoap2.o      # 622KB，较慢
g++ $F -c sys/status.cc       -o $D/status.o
g++ $F -c sql/sqlDbReq.cc     -o $D/sqlDbReq.o
g++ $F -I.. -c main/mainSoap.cc -o $D/mainSoap.o    # 注意 -I..（LinxSrvc 上级头文件）
g++ -o $D/gSOAPverify $D/*.o -lmysqlclient -lpthread
```

用 `make` 也可以，但有三个坑：① 没有 MySQL 库时不产出可执行文件却仍打印 `SUCCESS`；
② `mv $(CUR)/../*.o` 会搬走 `LinxSrvc/` 下**其它项目**的 `.o`；③ 别随手跑 `make all/install`
（会 `killall -9`、前台启动服务、写 `/usr/sbin` 与 `/etc/rc3.d`）。
`gSOAPverify.vcxproj` 的 `Release|x64` 配置为空、必然链接失败，**请用 Makefile**。

## 5. 数据库（仅 `login-by-key`）

```bash
sudo apt install mariadb-server libmysql++-dev
sudo systemctl start mariadb
mysql -uroot -p < file/myautomatic.sql              # 库 myautomatic、表 glkline
export MYAUTO_MYSQL_PSW='<root 密码>'               # sql/sqlDbReq.cc:27 读取
```

表 `glkline`：`idx`(AI) / `user` / `psw` / `tell` / `email` / 其余字段。
样例数据：`ioscatchme / 9f626c9b7028d552`、`ccccc / 88888`。

连接参数 `localhost:3306`、`root`、`myautomatic` **硬编码在源码里**（`sql/sqlDbReq.cc:10-12`），
无配置文件。样例数据含真实手机号与邮箱，勿用于生产。

## 6. 启动与停止

```bash
cd ../bin                # 必须与 myweb.wsdl 同目录，否则 ?wsdl 404
./gSOAPverify 8800       # 唯一参数：端口（必填，无默认值，无校验）
killall -9 gSOAPverify   # 无信号处理，只能强杀
```

- 线程模型：1 个 accept 主线程 + **8 个 worker** + 1024 槽环形队列。
- socket 超时 = `TIMEOUT_SEC`（`main/mainSoap.h`，默认 20 秒），accept/recv/send 三处都设。
- `main()` 只 `fork` 一次，父进程 `waitpid` 阻塞 → 前台会挂住，请用
  `nohup ./gSOAPverify 8800 > srv.log 2>&1 &` 或 systemd（需自配 `WorkingDirectory`）。
- 日志打印输出 stdout/stderr，无文件、无轮转、无级别。

## 7. 接口

统一 POST 到服务根路径，`Content-Type: text/xml`，命名空间 `urn:myweb`，无需 `SOAPAction`。

### `trans` — 参数解析回显

`msg` 格式 `trans@k1=v1&k2=v2`（首段必须是 `trans`，最多 8 组，key/value 各限 16 字符，
`msg` 总长限 128）。响应只回显**第一个**参数，其余打到服务日志。

```bash
curl -s -X POST --data-binary @req.xml http://localhost:8800
```

```xml
<api:trans><msg>trans@usr=tom&amp;psw=123</msg></api:trans>   <!-- XML 里 & 要写 &amp; -->
→ <rtn>Param(0): usr[tom]</rtn>
```

出错时**正常返回**（不发 Fault）：`illegal command!` / `request uri empty!` /
`request uri too long!` / `request uri error!`。

### `get-server-status` — 服务器状态

`req` 必须等于 `1000`，返回空闲内存百分比；不等于 `1000` 时 `rsp` 为空。

```xml
<api:get-server-status><req>1000</req></api:get-server-status>  →  <rsp>7.0067</rsp>
```

### `login-by-key` — 登录认证

```xml
<api:login-by-key><usr>ioscatchme</usr><psw>9f626c9b7028d552</psw></api:login-by-key>
→ <rslt><flag>200</flag><tell>...</tell><email>...</email></rslt>
```

| `flag` | 含义 |
|---|---|
| 200 | 命中，`email`/`tell` 为库中资料 |
| -2 | 查询失败（DB 不可用 / 表缺失 / 用户或口令不匹配） |
| -3 | 参数缺失 |

## 8. 遗留问题（未修）

**正确性 / 并发**

- `sql/sqlDbReq.cc`：看门狗线程的 `mysql_ping`/`mysql_real_connect` **无锁**，与查询线程竞争同一连接；`call_cnt == 0` 的初始化判断也无锁 → 并发首调用可能重复 init、起多个看门狗线程。
- 全局只有一条 MySQL 连接，`sqlClose()` 从未被调用。
- `sys/status.cc`：`get_mem_stat` 的 `strdup` 从不释放，每次 `get-server-status` 泄漏 2 块。
- `main/mainSoap.cc:28`：打印 `serv->user`（从未赋值）→ 输出垃圾值。

**安全**

- 明文 HTTP，无 TLS、无认证、无访问控制。
- 明文口令与完整 SQL 打印到 stdout（**按需要保留**，调试用；上生产前自行关闭）。
- `sql/sqlDbReq.cc` 的 `get_rslt_raw()` 含直接 SQL 注入，被 `#ifndef FIX` 排除，但是死代码仍在仓库，建议删除。
- 无信号处理、无 `chdir("/")`、无 stdio 重定向。

**工程**

- Makefile：见第 4 节三个坑；`-I/usr/lib$(BIT)/mysql` 对 Debian/Ubuntu 无效。
- `sys/status.cc` 的 `detect_eth_cable()` 会写 `const` 字符串且无调用点（死代码）。
- 无单元测试、无 systemd unit、无日志轮转。

## 9. 改接口后重新生成代码

真正的输入是 `soap/rpcapi.h`（**不是** wsdl2h 生成的 `myweb.h`）：

```bash
cd soap && soapcpp2 -S rpcapi.h        # 服务端代码
wsdl2h -o myweb.h myweb.wsdl           # 由 WSDL 反推头文件（改契约时）
# 见 soap/make.sh
```

改动后要同步 `main/mainSoap.cc` 里对应的 `api__*` 实现签名。
