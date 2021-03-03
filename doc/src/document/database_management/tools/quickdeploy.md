quickDeploy.sh 是一个 SequoiaDB 巨杉数据库的快速部署工具。它可以用命令行的方式快速部署 SequoiaDB / SequoiaSQL-MySQL / SequoiaSQL-PostgreSQL 。

支持部署 SequoiaDB 集群在多台主机上，SequoiaSQL-MySQL / SequoiaSQL-PostgreSQL 仅支持单台主机。

权限需求
---

运行 quickDeploy.sh 命令的用户必须是安装 SequoiaDB / SequoiaSQL-MySQL / SequoiaSQL-PostgreSQL 时指定的用户。

工具使用说明
---

### 用法

quickDeploy.sh [ options ] ...

### 参数说明

**--help, -h**

  返回帮助信息
  
**--sdb**

  部署 SequoiaDB
  
**--mysql**

  部署 SequoiaSQL-MySQL
  
**--pg**

  部署 SequoiaSQL-PostgreSQL
  
**--cm \<sdbcm port\>**

  指定 sdbcm 端口号，默认为11790。当 sdbcm 为非默认端口号时，要求所有安装了 SequoiaDB 的主机 sdbcm 端口必须一致
  
**--mysqlPath \<mysql installation path\>**

  quickDeploy.sh 只支持部署一个 SequoiaSQL-MySQL。当机器上装有多个 SequoiaSQL-MySQL 时，指定一个 SequoiaSQL-MySQL 的安装路径。

  需要配合 --mysql 使用。
  
**--pgPath \<pg installation path\>**

  quickDeploy.sh 只支持部署一个 SequoiaSQL-PostgreSQL。当机器上装有多个 SequoiaSQL-PostgreSQL 时，指定一个 SequoiaSQL-PostgreSQL 的安装路径。

  需要配合 --pg 使用。

> **Note:**
> 
> - 当不指定 --sdb / --mysql / --pg 参数时，quickDeploy.sh 会自动确认本机是否安装了 SequoiaDB / SequoiaSQL-MySQL / SequoiaSQL-PostgreSQL，已安装了的会自动部署

默认部署
---

### SequoiaDB 

SequoiaDB 默认部署到本机上：1 个协调节点，1 个编目节点，3 个数据组，数据组都是单副本。

```lang-bash
$ cd /opt/sequoiadb
$ ./tools/deploy/quickDeploy.sh --sdb
Execute command: /opt/sequoiadb/./tools/deploy/../../bin/sdb -f /opt/sequoiadb/./tools/deploy/quickDeploy.js -e 'var sdb=true;'

************ Deploy SequoiaDB ************************
Create catalog: ubuntu-200-091:11800
Create coord:   ubuntu-200-091:11810
Create data:    ubuntu-200-091:11820
Create data:    ubuntu-200-091:11830
Create data:    ubuntu-200-091:11840
$
$ ./bin/sdblist -l
Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
sequoiadb  11800         catalog     9180      1      1      Y    SYSCatalogGroup      2019-05-13-10.43.43  /opt/sequoiadb/database/catalog/11800/
sequoiadb  11810         coord       9571      2      2      Y    SYSCoord             2019-05-13-10.43.52  /opt/sequoiadb/database/coord/11810/
sequoiadb  11820         data        9646      1000   1000   Y    group1               2019-05-13-10.43.53  /opt/sequoiadb/database/data/11820/
sequoiadb  11830         data        9833      1001   1001   Y    group2               2019-05-13-10.43.57  /opt/sequoiadb/database/data/11830/
sequoiadb  11840         data        10061     1002   1002   Y    group3               2019-05-13-10.44.03  /opt/sequoiadb/database/data/11840/
Total: 5
```

### SequoiaSQL-MySQL

SequoiaSQL-MySQL 默认部署 myinst 实例，默认连接到 tools/deploy/sequoiadb.conf 中的第一个协调节点。

```lang-bash
$ ./quickDeploy.sh --mysql
Execute command: /opt/sequoiadb_yt/tools/deploy/./../../bin/sdb -f /opt/sequoiadb_yt/tools/deploy/./quickDeploy.js -e 'var mysql=true;'

************ Deploy SequoiaSQL-MySQL *****************
Create instance: [name: myinst, port: 3306]
```

### SequoiaSQL-PostgreSQL

SequoiaSQL-PostgreSQL 默认部署 myinst 实例，默认连接到 tools/deploy/sequoiadb.conf 中的第一个协调节点。

```lang-bash
$ ./quickDeploy.sh --pg
Execute command: /opt/sequoiadb_yt/tools/deploy/./../../bin/sdb -f /opt/sequoiadb_yt/tools/deploy/./quickDeploy.js -e 'var pg=true;'

************ Deploy SequoiaSQL-PostgreSQL ************
Create instance: [name: myinst, port: 5432]
```

在多台机器上部署
---

### SequoiaDB

以部署三机三组三节点为例：

+ 部署到三台机器上，主机名分别为 sdbserver1 / sdbserver2 / sdbserver3，请确保这三台主机都[安装](installation/deployment/command_installation/installation.md)了 SequoiaDB
+ 1 个协调节点组，每台机器上有一个协调节点
+ 1 个编目节点组，每台机器上有一个编目节点
+ 3 个数据节点组，组名分别为 group1 / group2 / group3，每个数据组有 3 个数据节点
+ 节点数据目录为安装路径下的 database 目录

> **Note:**
> 
> * 请先确保机器满足[软硬件要求](installation/system/system_requirement.md)
> * 请先参照[Linux推荐配置](installation/system/linux_suggest_settings.md)修改系统内核参数

1.  修改 tools/deploy/sequoiadb.conf :

  ```lang-csv
  role,groupName,hostName,serviceName,dbPath

  catalog,SYSCatalogGroup,sdbserver1,11800,[installPath]/database/catalog/11800
  catalog,SYSCatalogGroup,sdbserver2,11800,[installPath]/database/catalog/11800
  catalog,SYSCatalogGroup,sdbserver3,11800,[installPath]/database/catalog/11800

  coord,SYSCoord,sdbserver1,11810,[installPath]/database/coord/11810
  coord,SYSCoord,sdbserver2,11810,[installPath]/database/coord/11810
  coord,SYSCoord,sdbserver3,11810,[installPath]/database/coord/11810

  data,group1,sdbserver1,11820,[installPath]/database/data/11820
  data,group1,sdbserver2,11820,[installPath]/database/data/11820
  data,group1,sdbserver3,11820,[installPath]/database/data/11820

  data,group2,sdbserver1,11830,[installPath]/database/data/11830
  data,group2,sdbserver2,11830,[installPath]/database/data/11830
  data,group2,sdbserver3,11830,[installPath]/database/data/11830

  data,group3,sdbserver1,11840,[installPath]/database/data/11840
  data,group3,sdbserver2,11840,[installPath]/database/data/11840
  data,group3,sdbserver3,11840,[installPath]/database/data/11840
  ```

2.  部署 SequoiaDB

  ```lang-bash
  $ tools/deploy/quickDeploy.sh --sdb
  ```

  > **Note:**
  > 
  > - 找一台安装了 SequoiaDB 的机器执行以上命令即可

修改协调节点地址
---

### SequoiaSQL-MySQL

配置文件 tools/deploy/mysql.conf 属于 csv 格式，不同的配置参数以逗号分隔。

coordAddr 参数默认配置为 - ，会取 tools/deploy/sequoiadb.conf 中第一个 coord 的地址。

+ 指定具体的 coordAddr，格式为 localhost:50000

  ```lang-bash
  $ cat mysql.conf 
  instanceName,port,databaseDir,coordAddr
  myinst,3306,[installPath]/database/3306,localhost:50000
  ```

+ 指定多个协调节点地址，格式为 [localhost:50000,localhost:11810]

  ```lang-bash
  $ cat mysql.conf 
  instanceName,port,databaseDir,coordAddr
  myinst,3306,[installPath]/database/3306,[localhost:50000,localhost:11810]
  ```

### SequoiaSQL-PostgreSQL

配置文件 tools/deploy/postgresql.conf 属于 csv 格式，不同的配置参数以逗号分隔。

coordAddr 参数默认配置为 - ，会取 tools/deploy/sequoiadb.conf 中第一个 coord 的地址。

+ 指定具体的 coordAddr，格式为 localhost:50000

  ```lang-bash
  $ cat postgresql.conf 
  instanceName,port,databaseDir,coordAddr
  myinst,5432,[installPath]/database/5432,localhost:50000
  ```

+ 指定多个协调节点地址，格式为 [localhost:50000,localhost:11810]

  ```lang-bash
  $ cat postgresql.conf 
  instanceName,port,databaseDir,coordAddr
  myinst,5432,[installPath]/database/5432,[localhost:50000,localhost:11810]

部署多个 SQL 实例
---

### SequoiaSQL-MySQL

配置两个实例 myinst / myinst1，端口号分别为 3306 / 3307。

```lang-bash
$ cat mysql.conf 
instanceName,port,databaseDir,coordAddr
myinst,3306,[installPath]/database/3306,-
myinst1,3307,[installPath]/database/3307,-
```

### SequoiaSQL-PostgreSQL

配置两个实例 myinst / myinst1，端口号分别为 5432 / 5433。

```lang-bash
$ cat postgresql.conf 
instanceName,port,databaseDir,coordAddr
myinst,5432,[installPath]/database/5432,-
myinst1,5433,[installPath]/database/5433,-
```

