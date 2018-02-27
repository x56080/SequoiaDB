本入门教程使用 SequoiaDB 2.8.5 及 SequoiaSQL 2.8.5 在 Ubuntu 12.04 上搭建一个基础运行环境，以快速了解 SequoiaDB 及 SequoiaSQL 的基本功能 。

##安装 SequoiaDB##
安装过程需要使用操作系统 root 用户权限。

###安装介质准备###
从 [SequoiaDB 官网](http://www.sequoiadb.com/cn/index.php?a=index&m=Download) 下载 SequoiaDB 2.8.5 及 SequoiaSQL 2.8.5 的安装包，并上传到目标主机上。

###安装步骤###

- 以 root 用户登陆目标主机，解压 SequoiaDB 安装包 sequoiadb-2.8.5-linux_x86_64-installer.tar.gz，并给解压得到的 run 包增加可执行权限

  ```lang-javascript
  $ tar zxvf sequoiadb-2.8.5-linux_x86_64-installer.tar.gz 
  $ chmod u+x sequoiadb-2.8.5-linux_x86_64-installer.run
  ```
- 运行安装程序  
    
  ```lang-javascript
  $ ./sequoiadb-2.8.5-linux_x86_64-installer.run --mode text --SMS false
  ```

- 程序提示选择向导语言，输入2，选择中文

  ```
  Language Selection
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] :2
  ```

- 显示安装协议，直接按回车键忽略阅读并同意协议

  ```
  ------------------------------------------------------------
  由 BitRockInstallBuilder 评估本所建立
  ------------------------------------------------------------

  欢迎来到 SequoiaDB Server 安装程序


  重要信息：请仔细阅读

  下面提供了两个许可协议。

  1. SequoiaDB 评估程序的最终用户许可协议
  2. SequoiaDB 最终用户许可协议

  如果被许可方为了生产性使用目的（而不是为了评估、测试、试用“先试后买”或演示）获得本程序，单击下面的“接受”按钮即表示被许可方接受 SequoiaDB 最终用户许可协议，且不作任何修改。

  如果被许可方为了评估、测试、试用“先试后买”或演示（统称为“评估”）目的获得本程序：单击下面的“接受”按钮即表示被许可方同时接受（i）SequoiaDB 评估程序的最终用户许可协议（“评估许可”），且不作任何修改；和（ii）SequoiaDB 最终用户程序许可协议（SELA），且不作任何修改。

  在被许可方的评估期间将适用“评估许可”。

  如果被许可方通过签署采购协议在评估之后选择保留本程序（或者获得附加的本程序副本供评估之后使用），SequoiaDB 评估程序的最终用户许可协议将自动适用。

  “评估许可”和 SequoiaDB 最终用户许可协议不能同时有效；两者之间不能互相修改，并且彼此独立。

  这两个许可协议中每个协议的完整文本如下。

  评估程序的最终用户许可协议



  [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
  [2] 查看详细的协议内容
  请选择选项 [1] :
  ```

- 询问是否强制安装，直接按回车键选择否：

  ```
  ------------------------------------------------------------
  是否强制安装？强制安装时可能会强杀残留进程
  是否强制安装 [y/N]:
  ```

- 输入安装路径后按回车（可直接按回车使用默认路径 /opt/sequoiadb ）

  ```
  ------------------------------------------------------------
  请指定 SequoiaDB Server 将会被安装到的目录
  安装目录 [/opt/sequoiadb]:
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组），该用户名用于运行 SequoiaDB 服务，本次均直接按回车使用默认值

  ```
  ------------------------------------------------------------
  数据库管理用户配置
  配置用于启动 SequoiaDB 的用户名、用户组和密码
  用户名 [sdbadmin]:
  用户组 [sdbadmin_group]:
  ```

- 提示输入该用户的密码和确认密码（默认密码为 sdbadmin ），本次均直接按回车使用默认值

  ```
  密码 [********] :
  确认密码 [********] :
  ```

- 输入两次密码后，此时系统提示输入配置服务端口（默认为11790），直接按回车使用默认值

  ```
  ------------------------------------------------------------
  集群管理服务端口配置
  配置SequoiaDB集群管理服务端口，集群管理用于远程启动添加和启停数据库节点
  端口 [11790]:
  ```

- 询问是否允许 SequoiaDB 相关进程开机自启动，输入Y，按回车

  ```
  ------------------------------------------------------------
  是否允许 SequoiaDB 相关进程开机自启动
  Sequoiadb相关进程开机自启动 [Y/n]:
  ```

- 设置完成，询问是否继续安装，直接按回车选择是

  ```
  ----------------------------------------------------------------------------
  设定现在已经准备将 SequoiaDB Server 安装到您的电脑.
  您确定要继续? [Y/n]:
  ```
 
- 安装完成

  ```
  正在安装 SequoiaDB Server 于您的电脑中，请稍候。
  安装中
  0% ______________ 50% ______________ 100%
  #########################################
  ------------------------------------------------------------
  安装程序已经完成安装 SequoiaDB Server 于你的电脑中.
  ```

- 安装检查

  切换到 sdbadmin 用户，使用如下命令如能正常查到 SequoiaDB 的版本信息，说明安装成功。

  ```lang-javascript
  $ sequoiadb  --version
  SequoiaDB version: 2.8.5
  Release: 34299
  2018-02-12-12.24.46
  ```

##安装 SequoiaSQL##

- 以 root 用户登陆目标主机，解压 SequoiaSQL 安装包 sequoiasql-oltp-2.8.5-x86_64-installer.tar.gz，并给解压得到的 run 包增加可执行权限

  ```lang-javascript
  $ tar zxvf sequoiasql-oltp-2.8.5-x86_64-installer.tar.gz
  $ chmod u+x sequoiasql-oltp-2.8.5-x86_64-installer.run
  ```

- 运行安装程序  
    
  ```lang-javascript
  $ ./sequoiasql-oltp-2.8.5-x86_64-installer.run --mode text
  ```

- 程序提示选择向导语言，输入2，选择中文

  ```
  Language Selection
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] :2
  ```

- 输入安装路径后按回车（可直接按回车使用默认路径 /opt/sequoiasqloltp ）

  ```
  ----------------------------------------------------------------------------
  由BitRock InstallBuilder评估本所建立
  
  欢迎来到 SequoiaSQL Server 安装程序

  ------------------------------------------------------------
  请指定 SequoiaSQLServer 将会被安装到的目录
  安装目录 [/opt/sequoiasqloltp]:
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组），该用户名用于运行 SequoiaSQL 服务，本次均直接按回车使用默认值

  ```
  ------------------------------------------------------------
  数据库管理用户配置
  配置用于启动 SequoiaSQL 的用户名、用户组和密码
  用户名 [sdbadmin]:
  用户组 [sdbadmin_group]:
  ```

- 提示输入该用户的密码和确认密码（默认密码为 sdbadmin ），本次均直接按回车使用默认值

  ```
  密码 [********]:
  确认密码 [********]:
  ```

- 设置完成，询问是否继续安装，直接按回车选择是

  ```
  ------------------------------------------------------------
  设定现在已经准备将 SequoiaSQL Server 安装到您的电脑.
  您确定要继续? [Y/n]: 
  ```
    
- 安装完成

  ```
  正在安装 SequoiaSQL Server 于您的电脑中，请稍候。
  安装中
  0% ______________ 50% ______________ 100%
  ########################################
  添加了系统服务: Ssql-oltp.
  #
  ------------------------------------------------------------
  安装程序已经完成安装 SequoiaSQL Server 于你的电脑中.
  ```

- 安装检查

  切换到 sdbadmin 用户，使用如下命令如能正常查到 SequoiaSQL 的版本信息，说明安装成功。

  ```lang=java-script
  $ sdb_sql_ctl --version
  2.8.5
  ```

## 操作环境准备 ##

### 创建实例及数据库 ###

1. 创建 SequoiaDB 实例

   以 sdbadmin 用户登陆 SequoiaDB 所在主机，使用如下命令创建一个 SequoiaDB 的单机环境，并启动节点

   ```lang-javascript
   $ sdb
   Welcome to SequoiaDB shell!
   help() for help, Ctrl+c or quit to exit
   > var oma = new Oma("localhost", 11790)
   Takes 0.004115s.
   > oma.createData(11810, "/opt/sequoiadb/database/standalone/11810")
   Takes 0.001889s.
   > oma.startNode(11810)
   Takes 13.195910s.
   ```

2. 创建 SequoiaSQL 实例

- 以 sdbadmin 用户登陆 SequoiaSQL 所在主机，切换到 SequoiaSQL 安装目录

  ```lang-javascript
  $ cd /opt/sequoiasqloltp
  ```

- 创建实例，指定实例名为myinst，数据目录为 pg_data，出现 ok 表示实例创建成功

   ```lang-javascript
   $ sdb_sql_ctl addinst myinst -D pg_data/
   Adding instance myinst ...
   ok
   ```

- 启动实例进程

   ```lang-javascript
   $ sdb_sql_ctl start myinst
   Starting instance myinst ...
   ok (PID: 20502)
   ```
    
- 查看实例状态
    
    ```lang-javascript
   $ sdb_sql_ctl status
   INSTANCE   PID      SVCNAME   PGDATA                        PGLOG                                   
   myinst     20502    5432      /opt/sequoiasqloltp/pg_data   /opt/sequoiasqloltp/pg_data/myinst.log     
   Total: 1; Run: 1
   ```

- 检查 SequoiaSQL 是否启动成功

   ```lang-javascript
   $ netstat -nap | grep 5432
   tcp   0   0 127.0.0.1:5432     0.0.0.0:*         LISTEN     20502/postgres
   unix  2   [ ACC ]   STREAM    LISTENING   40776754 20502/postgres     /tmp/.s.PGSQL.5432
   ```

- 创建 SequoiaSQL 的 database

   ```lang-javascript
   $ bin/createdb -p 5432 foo
   ```

### 配置 SequoiaSQL 与 SequoiaDB 连接 ###

- 进入 SequoiaSQL shell 环境

   ```lang-javascript
   $ cd /opt/sequoiasqloltp
   $ bin/psql -p 5432 foo
   psql (9.3.4)
   Type "help" for help.
   foo=#
   ```

- 加载 SequoiaDB 连接驱动

   ```lang-javascript
   foo=# create extension sdb_fdw;
   REATE EXTENSION
   ```
  
- 配置与SequoiaDB连接参数

   ```lang-javascript
   foo=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbadmin', password 'mypassword', preferedinstance 'A', transaction 'off');
CREATE SERVER
   ```

## 数据库操作 ##

### 使用 SequoiaSQL shell 进行 SQL 操作 ###

- 使用 sdbadmin 用户登陆主机

- 进入 SequoiaDB shell 环境并创建集合 foo.bar

   ```lang-javascript
   $ cd /opt/sequoiadb
   $ bin/sdb
   Welcome to SequoiaDB shell!
   help() for help, Ctrl+c or quit to exit
   > db=new Sdb()
   localhost:11810
   Takes 0.117950s.
   > db.createCS('foo').createCL('bar')
   localhost:11810.foo.bar
   Takes 0.298361s.
   ```
- 进入 SequoiaSQL shell 环境，创建外表

   ```lang-javascript
   foo=# create foreign table test( id int, name text ) server sdb_server options ( collectionspace 'foo', collection 'bar' );
CREATE FOREIGN TABLE
   ```

- 使用 SQL 语句进行增删改查操作

   ```lang-javascript
   foo=# insert into test values(1, 'Tom');
   INSERT 0 1
   foo=# insert into test values(2, 'Jerry');
   INSERT 0 1
   foo=# select * from test;
    id | name  
   ----+-------
     1 | Tom
     2 | Jerry
   (2 rows)

   foo=# update test set name = 'Tim' where id = 1;
   UPDATE 1
   foo=# select * from test;
    id | name  
   ----+-------
     1 | Tim
     2 | Jerry
   (2 rows)

   foo=# delete from test where name = 'Jerry';
   DELETE 1
   foo=# select * from test;
    id | name 
   ----+------
     1 | Tim
   (1 row)
   ```

### 使用 SequoiaDB shell 进行数据库操作 ###

- 使用 sdbadmin 用户登陆主机，启动 SequoiaDB shell：

  ```lang-javascript
  $ /opt/sequoiadb/bin/sdb
  Welcome to SequoiaDB shell!
  help() for help, Ctrl+c or quit to exit
  >
  ```

- 创建一个新的 sdb 连接

  ```lang-javascript
  > db = new Sdb()
  ```

- 创建集合空间 cs

  ```lang-javascript
  > db.createCS("cs")
  ```

- 创建集合 cl

  ```lang-javascript
  > db.cs.createCL("cl")
  ```

- 向集合 cs.cl 中写入记录

  ```lang-javascript
   > db.cs.cl.insert({id:1, name:"Tom"})
   Takes 0.000679s.
   > db.cs.cl.insert({id:2, name:"Jerry"})
   Takes 0.000447s.
  ```

- 查询结果

  ```lang-javascript
  > db.cs.cl.find()
  {
    "_id": {
      "$oid": "5a93bd4bc8ddfc8f28000001"
    },
    "id": 1,
    "name": "Tom"
  }
  {
    "_id": {
      "$oid": "5a93bd52c8ddfc8f28000002"
    },
    "id": 2,
    "name": "Jerry"
  }
  Return 2 row(s).
  Takes 0.000742s.
  ```

- 修改记录并查询结果

  ```lang-javascript
  > db.cs.cl.update({$set:{name:"Tim"}}, {id:1})
  Takes 0.001411s.
  > db.cs.cl.find()
  {
    "_id": {
      "$oid": "5a93bd4bc8ddfc8f28000001"
    },
    "id": 1,
    "name": "Tim"
  }
  {
    "_id": {
      "$oid": "5a93bd52c8ddfc8f28000002"
    },
    "id": 2,
    "name": "Jerry"
  }
  Return 2 row(s).
  Takes 0.001261s.
  ```

- 删除记录并查询结果

  ```
  > db.cs.cl.remove({id:2})
  Takes 0.001756s.
  > db.cs.cl.find()
  {
    "_id": {
      "$oid": "5a93bd4bc8ddfc8f28000001"
    },
    "id": 1,
    "name": "Tim"
  }
  Return 1 row(s).
  Takes 0.000702s.
  ```

- 查看帮助

   ```lang-javascript
   > help()
   var db = new Sdb()                                 connect to database use default host 'localhost' and default port 11810
   var db = new Sdb('localhost',11810)                connect to database use specified host and port
   var db = new Sdb('ubuntu',11810,'','')             connect to database with username and password
   var db = new SecureSdb()                           connect to database securely use default host 'localhost' and default port 11810
   var db = new SecureSdb('localhost',11810)          connect to database securely use specified host and port
   var db = new SecureSdb('ubuntu',11810,'','')       connect to database securely with username and password
   var oma = new Oma()                                connect to om agent use default host 'localhost' and default port 11810
   var oma = new Oma('localhost',11810)               connect to om agent use specified host and port
   var oma = new Oma('ubuntu',11810,'','')            connect to om agent with username and password
   help(<method>)                                     help on specified method, e.g. help('createCS')
   oma.help()                                         help on om methods
   db.help()                                          help on db methods
   db.cs.help()                                       help on collection space cs
   db.cs.cl                                           access collection cl on collection space cs
   db.cs.cl.help()                                    help on collection cl
   db.cs.cl.find()                                    list all records
   db.cs.cl.find({a:1})                               list records where a=1
   db.cs.cl.find().help()                             help on find methods
   db.cs.cl.count().help()                            help on count methods
   print(x), println(x)                               print out x
   sleep(ms)                                          sleep macro seconds
   traceFmt(<type>,<in>,<out>)                        format trace input(in) to output(out) by type
   getErr(ret)                                        print error description for return code
   getLastError()                                     get last error number
   setLastError(<errno>)                              set last error number
   getLastErrMsg()                                    get last error detail information
   setLastErrMsg(<msg>)                               set last error detail information
   getLastErrObj()                                    get last error object information
   setLastErrObj(<obj>)                               set last error object information
   showClass([className])                             show all class name or class's function name
   forceGC()                                          force garbage collection of JS objects
   jsonFormat(<pretty>)                               Set BSON output format.When out of memory
                                                      error happen, we can use jsonFormat( false ) to
                                                      disable BSON formatted output.
   clear                                              clear the terminal screen
   history -c                                         clear the history
   quit                                               exit
Takes 0.000419s.
   ```