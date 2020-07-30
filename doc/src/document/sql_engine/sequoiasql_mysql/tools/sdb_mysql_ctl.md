sdb_mysql_ctl 是 MySQL 实例组件的管理工具。用户通过 sdb_mysql_ctl 既可以初始化、启动和停止实例，也可以修改实例的引擎配置参数。

##参数说明##

| 参数 | 描述 | 是否必填 |
| ---- | ---- | -------- |
| -h | 返回帮助说明 | 否 |
| -D | 指定数据库储存路径 | 是 |
| -l | 指定日志文件，默认在安装路径下，与实例名同名 | 否 |
| -p | 指定 MySQL 服务的监听端口，默认为 3306 | 否 |
| -f | 指定 pid 文件，默认为数据库储存路径下的 `mysqld.pid` | 否 |
| -s | 指定 mysqld.sock 文件，默认为数据库储存路径下的 `mysqld.sock` | 否 |
| -w | 指定本地连接 root 用户的密码 | 否 |
| -a | 客户端最大连接数，默认为 1024 | 否 |
| -e | 错误日志级别，默认为 3 | 否 |
| -v | 输出版本信息 | 否 |
| --print | 打印日志信息 | 否 |
| --baklog | 删除实例时是否备份日志文件 | 否 |
               
##使用说明##

运行 sdb_mysql_ctl 的用户必须与安装 SequoiaSQL-MySQL 时指定的用户一致。

**管理实例**

 * 创建实例

   sdb_mysql_ctl addinst \<INSTNAME\> \<-D DATADIR\> [-l LOGFILE] [--print] [-p PORT] [-f PIDFILE] [-s SOCKETFILE] [-w PASSWORD]
 
   添加一个 myinst 的实例，指定数据库存储路径为 `database/3306/`，指定密码为 123456
 
   ```lang-bash
   $ sdb_mysql_ctl addinst myinst -D database/3306/ -l database/myinst.log --print -p 3306 -f database/myinst.pid -s database/myinst.sock -w 123456 
   ```

 * 启动实例

   sdb_mysql_ctl start \<INSTNAME\> [--print]
 
   ```lang-bash
   $ sdb_mysql_ctl start myinst
   ```
 
 * 查看实例状态
 
   sdb_mysql_ctl status [INSTNAME]
 
   ```lang-bash 
   $ sdb_mysql_ctl status myinst
   ```
 
 * 重启实例
 
   sdb_mysql_ctl restart \<INSTNAME\>
 
   ```lang-bash
   $ sdb_mysql_ctl restart myinst
   ```

 * 停止实例
 
   sdb_mysql_ctl stop \<INSTNAME\> [--print]
 
   ```lang-bash
   $ sdb_mysql_ctl stop myinst 
   ```
 
 * 删除实例

   sdb_mysql_ctl delinst \<INSTNAME\> [--baklog]
 
   ```lang-bash
   $ sdb_mysql_ctl delinst myinst
   ```
 
 * 查看所有添加的实例

   ```lang-bash
   $ sdb_mysql_ctl listinst
   ```

 * 启动所有实例

   ```lang-bash
   $ sdb_mysql_ctl startall
   ```

 * 停止所有实例

   ```lang-bash
   $ sdb_mysql_ctl stopall
   ```

**修改实例的配置**

 通过 sdb_mysql_ctl 可修改指定实例的 SequoiaDB 引擎配置，各配置项说明可参考 SequoiaDB [引擎配置](sql_engine/sequoiasql_mysql/config.md#引擎配置)
 
 ```lang-text
 sdb_mysql_ctl chconf <INSTNAME> [-p PORT] [-e LEVEL] [-a MAX-CON]
                      [--sdb-conn-addr=ADDR] [--sdb-user=USER] [--sdb-passwd=PASSWD] [--sdb-auto-partition=BOOL] [--sdb-use-bulk-insert=BOOL]
                      [--sdb-bulk-insert-size=SIZE] [--sdb-use-autocommit=BOOL] 
                      [--sdb-debug-log=BOOL] [--sdb-token=TOKEN] [--sdb-cipherfile=PATH] [--sdb-error-level=ENUM] [--sdb-replica-size=SIZE]
                      [--sdb-use-transaction=BOOL] [--sdb-optimizer-options=SET]
                      [--sdb-rollback-on-timeout=BOOL] [--sdb-execute-only-in-mysql=BOOL]
                      [--sdb-selector-pushdown-threshold=THRESHOLD] [--sdb-alter-table-overhead-threshold=THRESHOLD]
 ``` 

**示例**

修改 myinst 实例的 SequoiaDB 连接地址
 
```lang-bash
$ sdb_mysql_ctl chconf myinst --sdb-conn-addr=sdbserver1:11810,sdbserver2:11810
```

修改过程中需要提供该数据库该实例用户的密码，若未设置 root 用户的密码，在提示密码时直接输入回车即可

```lang-text
Changing configure of instance myinst ...
Enter password:
ok
```