SequoiaSQL-MySQL 的架构使集群中的多个 MySQL 实例均为主机模式，都可对外提供读写服务。由于各实例的元数据均只存储在该实例本身，SequoiaSQL-MySQL 提供了元数据同步工具，用来保证 MySQL 服务的高可用。当一个 MySQL 实例退出后，连接该实例的应用可以切换到其它实例，获得对等的读写服务。

## MySQL 元数据同步工具架构 ##
MySQL 元数据同步工具的基本原理是 MySQL 服务进程通过审计插件输出审计日志，元数据同步工具从审计日志中提取 SQL 语句，连接到其它 MySQL 实例执行，以达到元数据同步的目的。包含元数据同步工具的集群架构如下。
![](sql_engine/sequoiasql_mysql/meta_sync.jpg)

在上图中，meta_sync 即同步工具进程，每一个 MySQL 实例都有一个对应的同步工具在运行。它独立于 MySQL 服务进程运行，对 MySQL 的审计日志文件 server_audit.log 进行分析处理。由于用户的业务数据存储于底层的 SequoiaDB 数据库集群中，因此只要 MySQL 层的元数据在各实例间完成同步，连接 MySQL 实例的客户端就可以访问到一致的数据，这就为 MySQL 服务提供了高可用能力。

## 元数据同步工具适用范围 ##
本工具需要与 SequoiSQL-MySQL 配套使用，可完成常见 DDL、DCL 命令的同步。由于审计插件的特定限制，该工具的使用存在如下约束：

+ 不支持在不同实例上并发对相同数据库对象进行 DDL 操作，会造成一致性问题，如修改同一张表的属性
+ 由于同步工具需要过滤掉其它同步工具同步到本实例的操作，因此，所有通过其它实例所在主机连接到本实例执行的操作产生的审计日志，都会被过滤掉（审计日志中会记录发起命令的客户端所在主机地址），包括用户或其它程序在其它实例所在主机连接到本实例执行的操作
+ 不支持在 DDL 语句中使用 '\r\n', '\n' 及 '\t'，会造成实例间元数据不一致
+ 不支持在语句内部使用注释，如在行末使用 "--" 增加注释
+ 只同步审计日志中标识返回码为 0 的操作，因此如果操作中涉及到多个数据库对象，且在部分对象上操作失败，导致最终返回码非 0，则该操作不会被同步，如同时 drop 多个表，但其中有些表并不存在
+ 目前不支持一台主机上多个 MySQL 实例间同步，且所有主机上的 MySQL 实例需使用相同的服务端口
+ 需要在每个 MySQL 环境中部署审计插件及同步工具
+ MySQL 实例服务器之间需要使用主机名互通
+ 支持同步 ALTER，CREATE，DECLARE，GRANT，REVOKE，FLUSH 操作，其它操作暂不支持
+ 支持 python2.7+，不支持 python3 版本
+ 需要使用 SequoiaSQL-MySQL 3.2.3 或以上版本

## 安装 ##
MySQL 元数据同步工具以 python 脚本的形式随 SequoiaSQL-MySQL 的安装包一起发布，在安装 SequoiaSQL-MySQL 的过程中会被一同安装，工具路径在 SequoiaSQL-MySQL 安装路径下的 tools 目录下，目录结构如下：

```shell
tools/
├── lib
│   └── server_audit.so
└── metaSync
    ├── config.sample
    ├── keywords.py
    ├── log.config.sample
    ├── meta_sync.py
    └── README.md
```

其中，meta_sync.py 为同步工具主程序，config.sample 为工具配置文件样例，log.config.sample 为日志配置文件样例，lib 下的动态库为 MySQL 的审计插件。由于 MySQL 原生的审计插件只在企业版中提供，社区版需要借助第三方提供的审计插件来实现审计功能。本文采用的是 MarriaDB 提供的 server_audit 插件。

### 审计插件安装 ###
如上所述，审计插件的动态库已经包含在 SequoiaSQL-MySQL 安装目录中的 tools/lib 目录下，需要将其安装到 MySQL 中。在此之前，需要先完成 MySQL 环境的搭建及实例启动，插件安装完成后再重启 MySQL 服务。所有的 MySQL 环境都需要完成该插件的安装及配置。具体的安装步骤如下：

+ 切换到 SequoiaSQL-MySQL 安装用户（默认为 sdbadmin）
+ 在所有 MySQL 实例上创建用于同步元数据的 MySQL 用户，并授予所有权限，用户名与密码在所有实例上保持一致。注意：此处使用的密码 'sdbadmin' 仅为示例，请根据需要自行设置安全的密码

   ```sql
   mysql> CREATE USER 'sdbadmin'@'%' IDENTIFIED BY 'sdbadmin';
   mysql> GRANT all on *.* TO 'sdbadmin'@'%' with grant option;
   ```

+ 将上述审计插件 server_audit.so 文件复制到 MySQL 安装目录中的 lib/plugin 目录下，并赋予 MySQL 运行用户的可执行权限
+ 修改 MySQL 实例的配置文件（SequoiaSQL-MySQL 实例的配置文件为数据路径下的 auto.cnf），在 mysqld 部分添加以下内容，审计日志文件路径请根据实际情况进行配置，并手动完成创建，如以下示例中的 auditlog 目录

   ```config
   # 加载审计插件
   plugin-load=server_audit=server_audit.so
   # 审计记录的审计，建议只记录需要同步的DCL和DDL操作
   server_audit_events=CONNECT,QUERY_DDL,QUERY_DCL
   # 开启审计
   server_audit_logging=ON
   # 审计日志路径及文件名
   server_audit_file_path=/opt/sequoiasql/mysql/database/auditlog/server_audit.log
   # 强制切分审计日志文件
   server_audit_file_rotate_now=OFF
   # 审计日志文件大小10MB，超过该大小进行切割，单位为byte
   server_audit_file_rotate_size=10485760
   # 审计日志保留个数，超过后会丢弃最旧的
   server_audit_file_rotations=999
   # 输出类型为文件
   server_audit_output_type=file
   # 限制每行查询日志的大小为100kb，若表比较复杂，对应的操作语句比较长，建议增大该值
   server_audit_query_log_limit=102400
   ```

+ 重启 MySQL 实例

   ```config
   sdb_mysql_ctl restart myinst
   ```

+ 检查审计日志文件目录，确保生成了审计日志文件 server_audit.log

至此，即完成了元数据同步工具的环境安装。

## 使用 ##
在完成安装后，还需要对其进行配置，包含工具的配置及日志的配置。以下各操作步骤也都要在 SequoiaSQL-MySQL 安装用户（默认为 sdbadmin）下完成。
### 工具配置项 ###
工具使用的配置文件名为 config。如果是全新安装，开始该文件是不存在的，需要从 config.sample 进行拷贝。如果是升级，则该文件应当已经存在。配置项如下：

```config
[mysql]
# mysql节点主机名，只能填主机名
hosts = sdb1,sdb2,sdb3
# mysql服务端口
port = 3306
# 密码类型，0代表密码为明文，1代表密码为密文，初次使用配置为 0，密码使用明文，工具启动后会自动加密并更新此处配置
mysql_password_type = 0
# mysql用户
mysql_user = sdbadmin
# mysql密码
mysql_password = sdbadmin
# mysql安装目录
install_dir = /opt/sequoiasql/mysql
# 审计日志存储目录
audit_log_directory = /opt/sequoiasql/mysql/database/auditlog
# 审计日志文件名
audit_log_file_name = server_audit.log

[execute]
# 同步间隔，取值范围：[1-3600]
interval_time = 5
# 出错时是否忽略，如为 false，会一直重试
ignore_error = true
# 出错的情况下，忽略前的重试次数，取值范围：[1-1000]
max_retry_times = 5
```

在该配置文件中，需要根据实际情况修改 [mysql] 下各配置的值，[execute] 下的各配置通常使用默认值即可。
### 日志配置项 ###
同步工具使用 python 的 logging 模块输出日志，配置文件为 log.config。如果是全新安装，开始该文件是不存在的，需要从 log.config.sample 拷贝。配置项如下（日志目录会自动创建）：

```
[loggers]
keys=root,ddlLogger
[handlers]
keys=rotatingFileHandler
[formatters]
keys=loggerFormatter
[logger_root]
level=INFO
handlers=rotatingFileHandler
[logger_ddlLogger]
level=INFO
handlers=rotatingFileHandler
qualname=ddlLogger
propagate=0

[handler_rotatingFileHandler]
class=logging.handlers.RotatingFileHandler
# 日志级别，支持ERROR,INFO,DEBUG
level=INFO
# 日志格式
formatter=loggerFormatter
# 第1个参数为运行日志文件名,路径对应的目录必须已存在
# 第2个参数为写入模式，值为'a+',不建议修改
# 第3个参数为日志文件大小，单位为byte
# 第4个参数为备份日志文件，即日志文件总数为10+1
args=('logs/run.log', 'a+', 104857600, 10)

[formatter_loggerFormatter]
format=%(asctime)s [%(levelname)s] [%(filename)s:%(lineno)s] %(message)s
datefmt=
```

通常情况下，该配置文件中的各配置项均使用默认值即可。

### 启动工具 ###
在完成所有配置后，在各实例所在主机的 sdbadmin 用户下，执行以下命令在后台启动同步工具

```config
python /opt/sequoiasql/mysql/tools/metaSync/meta_sync.py &
```

完成环境配置后，可通过在各实例进行少量 DDL 操作，进行简单的同步验证，验证完成后清理掉验证数据。

可以通过配置定时任务提供基本的同步工具监控，定期检查程序是否在运行，若进程退出了，会被自动拉起。配置命令如下（在 SequoiaSQL-MySQL 安装用户下配置）：

```bash
crontab -e
#每一分钟运行一次
*/1 * * * * /usr/bin/python /opt/sequoiasql/mysql/tools/metaSync/meta_sync.py >/dev/null 2>&1 &
```

其中 /opt/sequoiasql/mysql/tools/metaSync 为同步工具默认路径，/usr/bin/python 为系统 python 路径。如 SequoiaSQL-MySQL 或 python 安装路径与默认值不同，请对应修改上述命令中的相关路径。配置完成后，观察同步脚本是否能定时被拉起。

### 状态文件 ###
工具在正常运行后，会在与 config 文件相同的目录下，创建名为 sync.stat 的文本文件，用于记录同步状态，以便工具在重启后，能接着之前的处理进度继续工作。
该文件是在 3.2.4 版本中新增，早期版本进度信息也是存储于 config 文件中的。在从老版本升级到新版本后，会在第一次启动的时候自动进行升级，生成正确的配置文件和状态文件。状态文件的内容如下：

```
[status]
# 最后扫描文件的审计日志文件的 inode
file_inode = 4589549
# 文件中最后处理的行号
last_parse_row = 123
```

以上各值为示例值，会在运行过程中自动刷新。
> 注意：在工具正常运行期间，禁止手动修改该文件，否则可能造成同步中断。
