---
title: gSOAPverify Introduce
---

基于 **gSOAP 2.8.106** 的多线程 SOAP 服务端（RPC/encoded），提供 `trans`、
`get-server-status`、`login-by-key` 三个接口，后者查本机 MySQL 的
`myautomatic.glkline` 表。

> 实测环境：aarch64 / Ubuntu 22.04 / g++ 11.4。

---

## 1. 状态

| 能力 | 实测 |
|---|---|
| 编译链接 | 6 个源文件 + `-lmysqlclient -lpthread`，~910KB ELF |
| `GET /?wsdl` | HTTP 200（CWD 下要有 `myweb.wsdl`） |
| `trans` | `trans@usr=tom&psw=123` → `Param(0): usr[tom]` |
| `get-server-status` | `req=100\|ram\|cpu`：空闲内存% / 内存大小 / 核数与架构 |
| `login-by-key` | `flag`：200 命中 / -2 查询失败 / -3 参数缺失 |
| 优雅退出 | `SIGTERM` → 约 1 秒收尾，8 个 worker 全部终止 |

⚠️ 演示与内网可用；上生产前先看第 9 节「遗留问题（未修）」。

## 2. 目录

```text
gSOAPverify/
├── Makefile              # 构建（产物 ../bin/gSOAPverify）
├── main/                 # 【手写】main、线程池 + 环形队列、http_get、3 个接口
├── soap/rpcapi.h         # 【手写·关键】gSOAP 的真正输入头文件
├── soap/{soapC,soapServer}.cpp、soapH.h、soapStub.h   # 【生成】序列化 + 分派
├── soap/stdsoap2.cpp/.h  # 【引擎，内嵌源码】
├── soap/myweb.wsdl、api.xsd、myweb.nsmap   # 契约（ns = urn:myweb）
├── soap/myweb.*.{req,res}.xml              # 6 个示例报文
├── soap/make.sh          # 重新生成代码
├── sql/                  # 【手写】MySQL 连接、重连看门狗、查询
├── sys/                  # 【手写】内存 / CPU 统计
├── file/myautomatic.sql  # 建库建表 + 2 条样例数据
└── fix/                  # 遗留补丁，不参与编译
```

参与编译的只有 6 个文件：`soap/{soapC,stdsoap2,soapServer}.cpp`、
`sys/status.cc`、`sql/sqlDbReq.cc`、`main/mainSoap.cc`。

## 3. 依赖

| 依赖 | 必需 | 说明 |
|---|---|---|
| g++（C++11） | ✅ | 实测 11.4 |
| MySQL/MariaDB 客户端库 | ✅ | `<mysql/mysql.h>` + `-lmysqlclient` |
| pthread | ✅ | `-lpthread` |
| MySQL 服务端 | ⚠️ | 仅 `login-by-key` 需要 |
| OpenSSL / zlib | ❌ | 未启用 → 只有明文 HTTP |

包名：Debian/Ubuntu `libmysql++-dev`，RHEL `mariadb-devel`。

## 4. 编译

```bash
cd LinxSrvc/gSOAPverify && make    # 产物 ../bin/gSOAPverify，并同步 myweb.wsdl
make FLAG="-O2 -std=c++11"         # 生产构建：不带 -DNS_DBG，不打印口令与 SQL
make clean                         # 只删本项目 .o 与可执行文件
sudo make install                  # 拷到 /usr/sbin（PREFIX= 可覆盖）
```

中间产物在 `../out/gSOAPverify/`，只清理自己的文件。MySQL 头/库由
`pkg-config`（其次 `mysql_config`）探测，没装开发库时 `make` 直接报错退出。
手工编译时注意 `-I..`（引用 `LinxSrvc/include`）。
`gSOAPverify.vcxproj` 的 `Release|x64` 配置为空、必然链接失败，**请用 Makefile**。

## 5. 数据库（仅 `login-by-key`）

```bash
sudo apt install mariadb-server libmysql++-dev
sudo systemctl start mariadb
mysql -uroot -p < file/myautomatic.sql     # 库 myautomatic、表 glkline
export MYAUTO_MYSQL_PSW='<root 密码>'
```

表 `glkline`：`idx`(AI) / `user` / `psw` / `tell` / `email` / 其余字段；
样例数据 `ioscatchme / 9f626c9b7028d552`、`ccccc / 88888`（含真实手机号与邮箱，
勿用于生产）。连接参数 `localhost:3306`、`root`、`myautomatic` 硬编码在
`sql/sqlDbReq.cc` 顶部常量区，无配置文件；未设 `MYAUTO_MYSQL_PSW` 时跳过
连接，只提示一次。

## 6. 启停

```bash
cd ../bin                # 必须与 myweb.wsdl 同目录，否则 ?wsdl 404
./gSOAPverify 8800       # 唯一参数：端口（必填，无默认值、无校验）
kill -TERM <pid>         # 优雅退出：停看门狗 → 通知 worker → 关 MySQL 连接
kill -9  <pid>           # 强杀，不做收尾
```

- 线程模型：1 个 accept 主线程 + 8 个 worker + 1024 槽环形队列。
- socket 超时 `TIMEOUT_SEC`（`main/mainSoap.h`，默认 20 秒），
  accept/recv/send 三处都设。
- 前台会挂住（`main()` fork 一次，父进程 `waitpid`）：用
  `nohup ./gSOAPverify 8800 > srv.log 2>&1 &` 或 systemd；
  父进程把终止信号转发给子进程并回传其退出码。
- 子进程 `setsid()` 后不 `chdir("/")`：WSDL 按相对路径读，
  所以 systemd 必须配 `WorkingDirectory`。
- 日志输出到 stdout/stderr，无文件、无轮转、无级别。

systemd 单元参考（`/etc/systemd/system/gSOAPverify.service`）：

```ini
[Unit]
Description=gSOAPverify SOAP service
After=network.target mariadb.service

[Service]
Type=forking
WorkingDirectory=/home/<user>/MyAutomatic/LinxSrvc/bin
Environment=MYAUTO_MYSQL_PSW=<root 密码>
ExecStart=/home/<user>/MyAutomatic/LinxSrvc/bin/gSOAPverify 8800
KillSignal=SIGTERM
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
```

## 7. 接口

统一 POST 到根路径，`Content-Type: text/xml`，命名空间 `urn:myweb`，
无需 `SOAPAction`：

```bash
curl -s -X POST -H 'Content-Type: text/xml' \
     --data-binary @soap/myweb.trans.req.xml http://localhost:8800
```

### `trans` — 参数解析回显

`msg` 格式 `trans@k1=v1&k2=v2`：首段必须是 `trans`，最多 8 组，key/value
各限 16 字符，`msg` 总长限 128。响应只回显第一个参数，其余打到日志。

```xml
<api:trans><msg>trans@usr=tom&amp;psw=123</msg></api:trans>
→ <rtn>Param(0): usr[tom]</rtn>
```

XML 里的 `&` 必须写成 `&amp;`。出错时**正常返回**（不发 Fault）：
`illegal command!` / `request uri empty!` / `request uri too long!` /
`request uri error!`。

### `get-server-status` — 服务器状态

POST 与 GET 都支持，`req` 决定返回什么：

| `req` | 返回 | 实测样例 |
|---|---|---|
| `100` | 空闲内存百分比 | `4.4959` |
| `ram` | 物理内存总大小 | `ram=7607MB` |
| `cpu` | CPU 核数与架构 | `cpu=6 arch=aarch64` |

```xml
<api:get-server-status><req>ram</req></api:get-server-status>
→ <rsp>ram=7607MB</rsp>
```

地址栏写法（`@` 与 `&` 等价，`ram`/`cpu` 也可直接当查询键）：

```text
http://localhost:8800/?get-server-status@req=ram
http://localhost:8800/?get-server-status@cpu=1
→ 200 text/plain：ram=7607MB / cpu=6 arch=aarch64
```

`req` 取其它值时：POST 返回空 `rsp`，GET 返回 **400** +
`usage: ?get-server-status@req=100|ram|cpu`。除 `?wsdl` 与本接口外，
其它 GET 一律 405。

### `login-by-key` — 登录认证

```xml
<api:login-by-key>
  <usr>ioscatchme</usr><psw>9f626c9b7028d552</psw>
</api:login-by-key>
→ <rslt><flag>200</flag><tell>...</tell><email>...</email></rslt>
```

| `flag` | 含义 |
|---|---|
| 200 | 命中，`email`/`tell` 为库中资料 |
| -2 | 查询失败（DB 不可用 / 表缺失 / 用户或口令不匹配） |
| -3 | 参数缺失 |

## 8. 改接口后重新生成代码

真正的输入是 `soap/rpcapi.h`（**不是** wsdl2h 生成的 `myweb.h`）：

```bash
cd soap && soapcpp2 -S rpcapi.h     # 服务端代码
wsdl2h -o myweb.h myweb.wsdl        # 由 WSDL 反推头文件（改契约时）
# 见 soap/make.sh
```

改动后需要同步 `main/mainSoap.cc` 里对应的 `api__*` 实现签名。

## 9. 遗留问题（未修）

- **安全**：明文 HTTP —— 无 TLS、无认证、无访问控制、无速率限制。
- **日志**：`-DNS_DBG` 下明文口令与完整 SQL 打到 stdout，去掉该宏重新构建
  即关闭；日志无文件、无轮转、无级别。
- **并发**：全局只有一条 MySQL 连接，8 个 worker 的查询互相串行，高并发需
  改连接池；`sqlQuery` 的 `-4`（表名非法）/ `-5`（表不存在）都映射成 `flag=-2`。
- **进程**：端口参数无校验；不 `chdir("/")`、不重定向 stdio。
- **工程**：无单元测试、无日志轮转；systemd 单元按第 6 节片段自建。
- `sys/status.cc` 的 `detect_eth_cable()` 无调用点，仅作参考代码保留。
