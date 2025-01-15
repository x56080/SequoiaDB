
quickDeploy.sh 是 SequoiaDB 巨杉数据库的快速部署工具，用于部署 SequoiaDB 集群和 SQL 实例。其中，SequoiaDB 集群支持部署在单台或多台机器，SQL 实例仅支持部署在单台机器。

##语法规则##

**quickDeploy.sh [--sdb] [--mysql] [--mariadb] [--pg] [--cm=Number] [--mysqlPath=String] [--mariadbPath=String] [--pgPath=String]**

##参数说明##

|参数名  |缩写     | 描述   |
|--------|---------|--------|
|--help  |    -h   | 获取帮助信息 |
|--sdb   |    -    | 部署 SequoiaDB 集群 |
|--mysql |    -    | 部署 MySQL 实例 |
|--mariadb |    -  | 部署 MariaDB 实例 |
|--pg    |    -    | 部署 PostgreSQL 实例 |
|--cm    |    -    | 指定 sdbcm 端口号，默认值为 11790 <br>在多台机器上部署集群时，需确保所有机器的 sdbcm 端口一致 |
|--mysqlPath| -    | 指定 MySQL 实例组件的安装路径 |
|--mariadbPath| -  | 指定 MariaDB 实例组件的安装路径 |
|--pgPath|    -    | 指定 PostgreSQL 实例组件的安装路径 |

> **Note:**
> 
> - 当不指定参数 --sdb、--mysql、--mariadb 和 --pg 时，快速部署工具将根据本机的安装情况自动部署 SequoiaDB 集群和 SQL 实例。
> - 快速部署工具不支持同时部署多个 MySQL、MariaDB 或 PostgreSQL 实例组件。如果本机安装了多个 MySQL、MariaDB 或 PostgreSQL 实例组件，需指定参数 --mysqlPath、--mariadbPath 或 --pgPath。

##常见场景##

快速部署工具常用于伪集群或集群部署。伪集群部署是指在单台机器上部署集群，而集群部署是指在多台机器上部署集群。部署完成后，用户可使用 [sdblist][sdblist] 工具查看当前集群的部署情况。

###伪集群部署###

快速部署工具默认在当前机器中部署一个三组单副本的伪集群，同时根据已安装的实例组件创建 SQL 实例。以 MySQL 实例组件为例，具体步骤如下：

1. 切换至 SequoiaDB 安装目录

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 执行快速部署工具

    ```lang-bash
    $ ./tools/deploy/quickDeploy.sh
    ```

    输出如下信息则表示部署成功：

    ```lang-text
    ************ Deploy SequoiaDB ************************
    Create catalog: sdbserver1:11800
    Create coord:   sdbserver1:11810
    Create data:    sdbserver1:11820
    Create data:    sdbserver1:11830
    Create data:    sdbserver1:11840
    ************ Deploy SequoiaSQL-MySQL *****************
    Create instance: [name: myinst, port: 3306]
    ```

###集群部署###

快速部署工具通过配置文件实现集群部署。用户可根据业务需求，修改配置文件以调整集群规模。

配置文件位于目录 `<INSTALL_DIR>\tools\deploy`，其中 `sequoiadb.conf` 用于配置 SequoiaDB 集群，`mysql.conf`、`mariadb.conf` 和 `postgresql.conf` 用于配置 SQL 实例。下述以三台机器为例，部署一个三组三副本的集群，并创建多个 MySQL 实例。

1. 切换至 SequoiaDB 安装目录

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 编辑配置文件 `sequoiadb.conf`

    ```lang-bash
    $ vim tools/deploy/sequoiadb.conf
    ```

   修改内容如下：

    ```lang-text
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

3. 编辑配置文件 `mysql.conf`

    ```lang-bash
    $ vim tools/deploy/mysql.conf 
    ```

    分别配置端口为 3306 和 3307 的实例，修改内容如下：

    ```lang-text
    instanceName,port,databaseDir,coordAddr
    myinst1,3306,[installPath]/database/3306,-
    myinst2,3307,[installPath]/database/3307,sdbserver1:11810
    ```

    >**Note:**
    >
    > - 符号“-”表示 `sequoiadb.conf` 中第一个协调节点的地址。
    > - 如果需要指定多个协调地址，可使用逗号（,）间隔，例如 `[sdbserver1:11810,sdbserver2:11810]`。

4. 执行快速部署工具

    ```lang-bash
    $ ./tools/deploy/quickDeploy.sh
    ```

    输出如下内容则表示部署成功：

    ```lang-text

    ************ Deploy SequoiaDB ************************
    Create catalog: sdbserver1:11800
    Create catalog: sdbserver2:11800
    Create catalog: sdbserver3:11800
    ...
    Create data:    sdbserver1:11840
    Create data:    sdbserver2:11840
    Create data:    sdbserver3:11840
    
    ************ Deploy SequoiaSQL-MySQL *****************
    Create instance: [name: myinst1, port: 3306]
    Create instance: [name: myinst2, port: 3307]
    ```

[^_^]:
    本文使用的所有引用及链接
[env_requirement]:manual/Deployment/env_requirement.md
[linux_suggest_settings]:manual/Deployment/linux_suggestion.md
[sdblist]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdblist.md
