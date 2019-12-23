本入门教程使用 SequoiaDB 3.2 及 MySQL 实例组件3.2 在 Ubuntu 16.04 上搭建一个基础运行环境，以快速了解 SequoiaDB 及 MySQL 实例组件的基本功能 。

SequoiaDB 可以选择部署在单台机器上，也可以部署在多台机器上。

## 安装 SequoiaDB 及 MySQL 实例组件

### 安装前准备

- 下载 [SequoiaDB 最新数据库安装包](http://download.sequoiadb.com/cn/sequoiadb_latest)，并上传到目标主机上

  ```lang-bash
  $ wget --content-disposition http://download.sequoiadb.com/cn/sequoiadb_latest
  ```
- 安装过程需要使用操作系统 root 用户权限
- 确保系统满足[硬件和软件要求](installation/system/system_requirement.md)
- 参考[Linux推荐配置](installation/system/linux_suggest_settings.md)章节配置好相关系统参数
- 请确保所有主机都设置了[主机名](installation/system/system_requirement.md#配置主机名)，并且都设置了[主机名/IP地址映射关系](installation/system/system_requirement.md#配置主机名IP地址映射)
- 如果要部署 SequoiaDB 到多台机器上，每台机器必须都安装 SequoiaDB

### 安装步骤

- 以 root 用户登陆目标主机，解压 SequoiaDB 安装包 sequoiadb-3.2-linux_x86_64.tar.gz，并给解压得到的 run 包增加可执行权限

  ```lang-bash
  # tar zxvf sequoiadb-3.2-linux_x86_64.tar.gz 
  # cd sequoiadb-3.2
  # chmod u+x sequoiadb-3.2-linux_x86_64-installer.run
  # chmod u+x sequoiasql-mysql-3.2-linux_x86_64-installer.run
  # chmod u+x sequoiasql-postgresql-3.2-x86_64-installer.run
  # chmod u+x setup.sh
  ```

- 运行安装脚本

  ```lang-bash
  # ./setup.sh
  ```

- 程序提示选择安装SequoiaDB，默认是安装，输入N不安装

  ```
  Install sequoiadb Y/n: 
  ```

- 程序提示开始安装SequoiaDB，选择向导语言，输入2，选择中文

  ```
  ----------------------------begin to install sequoiadb----------------------------
  ./sequoiadb-3.2-linux_x86_64-installer.run --mode text
  Language Selection
  
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] : 2
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

- 输入安装路径后按回车（可直接按回车使用默认路径 /opt/sequoiadb ）

  ```
  ------------------------------------------------------------
  请指定 SequoiaDB Server 将会被安装到的目录
  安装目录 [/opt/sequoiadb]:
  ```

- 询问是否强制安装，直接按回车键选择否：

  ```
  ------------------------------------------------------------
  是否强制安装？强制安装时可能会强杀残留进程
  是否强制安装 [y/N]:
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

- 询问是否安装 OM 服务，输入Y表示安装，默认不安装，按回车

  ```
  ----------------------------------------------------------------------------
  
  是否安装OM服务 [y/N]: 
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
  
  ----------------------------end install sequoiadb----------------------------
  ```

- 安装 SequoiaSQL，询问安装 sequoiasql-mysql or [sequoiasql-postgresql](sql_engine/sequoiasql_pg/install/install_deploy.md)，分别用 1 和 2 表示，默认是 1，回车安装 sequoiasql-mysql

  ```
  Install 1:sequoiasql-mysql or 2:sequoiasql-postgresql, [1]: 
  ```

- 程序提示选择向导语言，输入2，选择中文

  ```
  --------------------------begin to install sequoiasql-mysql-------------------------
  ./sequoiasql-mysql-3.2-linux_x86_64-installer.run --mode text
  Language Selection
  
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] : 2
  ```

- 显示安装协议，直接按回车键忽略阅读并同意协议

  ```
  ----------------------------------------------------------------------------
  由BitRock InstallBuilder评估本所建立
  
  欢迎来到 MySQL 实例安装程序
  
  ----------------------------------------------------------------------------
  GNU 通用公共授权
  第二版, 1991年6月
  著作权所有 (C) 1989，1991 Free Software Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA   02111-1307 USA.
  允许每个人复制和发布本授权文件的完整副本，但不允许对它进行任何修改。
  
  [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
  [2] 查看详细的协议内容
  请选择一个选项 [1] : 
  ```

- 输入安装路径后按回车（可直接按回车使用默认路径 /opt/sequoiasql/mysql ）

  ```
  ----------------------------------------------------------------------------
  请指定 MySQL 实例将会被安装到的目录
  
  安装目录 [/opt/sequoiasql/mysql]: 
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组），该用户名用于运行 SequoiaSQL 服务，本次均直接按回车使用默认值

  ```
  ----------------------------------------------------------------------------
  数据库管理用户配置
  
  配置用于启 MySQL 实例组件的用户名、用户组和密码
  
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
  ----------------------------------------------------------------------------
  设定现在已经准备将 MySQL 实例安装到您的电脑.
  
  您确定要继续? [Y/n]: 
  ```

- 安装完成

  ```
  ----------------------------------------------------------------------------
  正在安装 MySQL 实例于您的电脑中，请稍候.
  
   安装中
   0% ______________ 50% ______________ 100%
   #########################################
  
  ----------------------------------------------------------------------------
  安装程序已经完成安装 MySQL 实例于你的电脑中.
  
  ----------------------------end install sequoiasql-mysql----------------------------
  ```

- 安装检查

  切换到 sdbadmin 用户，使用如下命令如能正常查到 SequoiaDB 的版本信息，说明安装成功。

  ```lang-bash
  $ sequoiadb  --version
  SequoiaDB shell version: 3.2
  Release: 40381
  2019-04-13-08.37.10
  ```

  

  切换到 root 用户，使用如下命令如能正常查到 sequoiasql-mysql 服务的状态，说明安装成功。

  ```lang-bash
  # service sequoiasql-mysql status
  Status of service sequoiasql-mysql: 
  running. (PID: 1493)
  ```



## 部署 SequoiaDB 及 MySQL 实例

SequoiaDB 部署方案可以选择部署在单台机器上，或者部署在多台机器上。下面介绍的伪集群部署和集群部署， **只需要二者选一种执行**。

### 伪集群部署

部署 SequoiaDB 到本机上，创建 3 个数据组，每个数据组单副本。在本机上创建一个MySQL实例。

![伪集群部署](quickstart_1.png)

- 使用 root 用户或者管理员用户登录主机

- 查看端口是否被占用

  执行以下命令查看 11800 端口是否被占用：

  ```lang-bash
  # netstat -anp | grep 11800
  ```

- SequoiaDB 默认需要的端口号为 11800、11810、11820、11830、11840、18800，MySQL 实例默认需要的端口号为 3306。请确保这些端口没有被占用。

- 使用 sdbadmin 用户登录主机

- 快速部署

  ```lang-bash
  $ # 切换到 SequoiaDB 安装目录下
  $ cd /opt/sequoiadb 
  $ ./tools/deploy/quickDeploy.sh
  
  ************ Deploy SequoiaDB ************************
  Create catalog: sdbserver1:11800
  Create coord:   sdbserver1:11810
  Create data:    sdbserver1:11820
  Create data:    sdbserver1:11830
  Create data:    sdbserver1:11840
  
  ************ Deploy SequoiaSQL-MySQL *****************
  Create instance: [name: myinst, port: 3306]
  ```

  > **Note:**  
  >
  > * 快速部署工具的使用与配置，具体请参考 [quickDeploy.sh](database_management/tools/quickdeploy.md)  
  >
  > * 错误处理：如果执行快速部署工具的过程中，发生异常，如端口占用，在恢复异常后，即端口可用后，可尝试再次执行 quickDeploy.sh 命令。如果再次执行 quickDeploy.sh 也失败，请先用解压目录下的 [clean.sh](quickstart_clean.md) 清除所有的安装部署，重头按快速入门教程执行命令。

### 集群部署

部署 SequoiaDB 到 3 台机器上，主机名分别为 sdbserver1 / sdbserver2 / sdbserver3，创建 3 个数据组，每个数据组 3 副本。在 sdbserver1 上创建一个MySQL实例。

![集群部署](quickstart_2.png)

- 使用 root 用户或者管理员用户登录主机

- 查看端口是否被占用

  在每台主机上执行以下命令查看 11800 端口是否被占用：

  ```lang-bash
  # netstat -anp | grep 11800
  ```

  SequoiaDB 需要的端口号为 11800、11810、11820、11830、11840、18800，MySQL 实例默认需要的端口号为 3306。请确保这些端口没有被占用。

- 使用 sdbadmin 用户登录主机

- 修改配置

  修改第一台主机 sdbserver1 上的 配置文件 tools/deploy/sequoiadb.conf，如下 :

  ```lang-ini
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

  用户机器的主机名不是 sdbserver1 / sdbserver2 / sdbserver3 的，只需要替换上面的 sdbserver1 / sdbserver2 / sdbserver3 即可。

- 快速部署

  在主机 sdbserver1 上执行快速部署工具

  ```lang-bash
  $ # 切换到 SequoiaDB 安装目录下
  $ cd /opt/sequoiadb
  $ ./tools/deploy/quickDeploy.sh
  
  ************ Deploy SequoiaDB ************************
  Create catalog: sdbserver1:11800
  Create catalog: sdbserver2:11800
  Create catalog: sdbserver3:11800
  Create coord:   sdbserver1:11810
  Create coord:   sdbserver2:11810
  Create coord:   sdbserver3:11810
  Create data:    sdbserver1:11820
  Create data:    sdbserver2:11820
  Create data:    sdbserver3:11820
  Create data:    sdbserver1:11830
  Create data:    sdbserver2:11830
  Create data:    sdbserver3:11830
  Create data:    sdbserver1:11840
  Create data:    sdbserver2:11840
  Create data:    sdbserver3:11840
  
  ************ Deploy SequoiaSQL-MySQL *****************
  Create instance: [name: myinst, port: 3306]
  ```

  > **Note:**  
  >
  > * 快速部署工具的使用与配置，具体请参考 [quickDeploy.sh](database_management/tools/quickdeploy.md)  
  >
  > * 错误处理：如果执行快速部署工具的过程中，发生异常，如端口占用，在恢复异常后，即端口可用后，可尝试再次执行 quickDeploy.sh 命令。如果再次执行 quickDeploy.sh 也失败，请先用解压目录下的 [clean.sh](quickstart_clean.md) 清除所有的安装部署，重头按快速入门教程执行命令。


## 使用 MySQL shell 进行操作

- 使用 sdbadmin 用户登录主机

- 登录 MySQL shell

  ```lang-bash
  $ /opt/sequoiasql/mysql/bin/mysql -h 127.0.0.1 -P 3306 -u root
  ```

- 创建数据库实例

  ```lang-sql
  mysql> create database cs;
  Query OK, 1 row affected (0.00 sec)
  
  mysql> use cs;
  Database changed
  ```

- 创建表

  ```lang-sql
  mysql> create table cl(a int, b int, c text, primary key(a, b) ) ;
  Query OK, 0 rows affected (0.66 sec)
  ```

- 使用 SQL 语句进行增删改查操作

  ```lang-sql
  mysql> insert into cl values(1, 101, "SequoiaDB test");
  Query OK, 1 row affected (0.05 sec)
  
  mysql> insert into cl values(2, 102, "SequoiaDB test");
  Query OK, 1 row affected (0.01 sec)
  
  mysql> select * from cl order by b asc;
  +---+-----+----------------+
  | a | b   | c              |
  +---+-----+----------------+
  | 1 | 101 | SequoiaDB test |
  | 2 | 102 | SequoiaDB test |
  +---+-----+----------------+
  2 rows in set (0.00 sec)
  
  mysql> update cl set c="My test" where a=1;
  Query OK, 1 row affected (0.01 sec)
  Rows matched: 1  Changed: 1  Warnings: 0
  
  mysql> delete from cl where b=102;
  Query OK, 1 row affected (0.02 sec)
  
  mysql> select * from cl order by b asc;
  +---+-----+---------+
  | a | b   | c       |
  +---+-----+---------+
  | 1 | 101 | My test |
  +---+-----+---------+
  1 row in set (0.00 sec)
  ```

如需一键清理安装的所有 SequoiaDB、MySQL 实例组件以及 PostgreSQL 实例组件和数据，前往[快速入门之一键清理](quickstart_clean.md)。
