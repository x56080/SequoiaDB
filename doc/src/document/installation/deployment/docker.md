

为方便用户快速体验，SequoiaDB 巨杉数据库提供基于 Docker 的镜像。本文介绍如何在 Docker 环境下部署 SequoiaDB 分布式集群环境。



## 集群规划

用户可以在五个容器中部署一个多节点高可用 SequoiaDB 集群。

| 主机名          | IP               | 分区组          | 部署软件               |
| --------------- | ---------------- | --------------- | ---------------------- |
| Coord 协调节点  | 172.17.0.2:11810 | SYSCoord        | SequoiaDB 3.2.1        |
| Catalog编目节点 | 172.17.0.2:11800 | SYSCatalogGroup | SequoiaDB 3.2.1        |
| Data1数据节点1  | 172.17.0.3:11820 | group1          | SequoiaDB 3.2.1        |
| Data2数据节点2  | 172.17.0.4:11820 | group1          | SequoiaDB 3.2.1        |
| Data3数据节点3  | 172.17.0.5:11820 | group1          | SequoiaDB 3.2.1        |
| Data1数据节点2  | 172.17.0.4:11830 | group2          | SequoiaDB 3.2.1        |
| Data2数据节点3  | 172.17.0.5:11830 | group2          | SequoiaDB 3.2.1        |
| Data3数据节点1  | 172.17.0.3:11830 | group2          | SequoiaDB 3.2.1        |
| Data1数据节点3  | 172.17.0.5:11840 | group3          | SequoiaDB 3.2.1        |
| Data2数据节点1  | 172.17.0.3:11840 | group3          | SequoiaDB 3.2.1        |
| Data3数据节点2  | 172.17.0.4:11840 | group3          | SequoiaDB 3.2.1        |
| MySQL实例       | 172.17.0.6:3306  | -               | SequoiaSQL-MySQL 3.2.1 |

集群包含一个协调节点与编目节点，三个三副本数据节点，与一个 MySQL 实例节点。



## 样例环境

| Docker 环境      | Mac Docker 2.0.0.3                                           |
| ---------------- | ------------------------------------------------------------ |
| 容器操作系统版本 | Ubuntu 18                                                    |
| 数据库版本       | SequoiaDB 3.2.3                                              |
| 集群部署         | 一个运行协调和编目节点，三个运行数据节点，一个运行 MySQL 实例 |

Docker 在 Linux/Windows/MacOS 平台安装方法可参考官方文档。

对于 Linux 环境可参考本样例安装 Docker 环境。

```lang-bash
$ apt-get install -y docker.io
```



## 拉取镜像

```lang-bash
$ docker pull sequoiadb/sequoiadb
$ docker pull sequoiadb/sequoiasql-mysql
```



## 启动四个 SequoiaDB 容器

```lang-bash
$ docker run -it -d --name coord_catalog sequoiadb/sequoiadb:latest
$ docker run -it -d --name sdb_data1 sequoiadb/sequoiadb:latest
$ docker run -it -d --name sdb_data2 sequoiadb/sequoiadb:latest
$ docker run -it -d --name sdb_data3 sequoiadb/sequoiadb:latest
```

查看四个容器的容器 ID

```lang-bash
$ docker ps -a | awk '{print $NF}';
```

运行结果：

```lang-bash
NAMES
sdb_data3
sdb_data2
sdb_data1
coord_catalog
```

查看四个容器的容器对应的 IP 地址

```lang-bash
$ docker inspect coord_catalog | grep IPAddress |awk 'NR==2 {print $0}'
$ docker inspect sdb_data1 | grep IPAddress |awk 'NR==2 {print $0}'
$ docker inspect sdb_data2 | grep IPAddress |awk 'NR==2 {print $0}'
$ docker inspect sdb_data3 | grep IPAddress |awk 'NR==2 {print $0}'
```

四条命令的输出结果分别为各个容器自身的 IP 地址：

```lang-bash
            "IPAddress": "172.17.0.2",
            "IPAddress": "172.17.0.3",
            "IPAddress": "172.17.0.4",
            "IPAddress": "172.17.0.5",           
```

## 部署 SequoiaDB 集群

根据集群规划以及各个容器的 IP 地址，在对应参数填入各自的地址与端口号。

```lang-bash
$ docker exec coord_catalog "/init.sh" \
      --coord='172.17.0.2:11810' \
      --catalog='172.17.0.2:11800' \
      --data='group1=172.17.0.3:11820,172.17.0.4:11820,172.17.0.5:11820;group2=172.17.0.4:11830,172.17.0.5:11830,172.17.0.3:11830;group3=172.17.0.5:11840,172.17.0.3:11840,172.17.0.4:11840'
```

该命令输出结果为：

```lang-bash
Begin generating SequoiaDB conf file
Finish generating SequoiaDB conf file
Restarting sdbcm process, it will take 10 seconds
Deploy...
Execute command: /opt/sequoiadb/tools/deploy/../../bin/sdb -f /opt/sequoiadb/tools/deploy/quickDeploy.js -e ''

************ Deploy SequoiaDB ************************
Create catalog: 172.17.0.2:11800
Create coord:   172.17.0.2:11810
Create data:    172.17.0.3:11820
Create data:    172.17.0.4:11820
Create data:    172.17.0.5:11820
Create data:    172.17.0.4:11830
Create data:    172.17.0.5:11830
Create data:    172.17.0.3:11830
Create data:    172.17.0.5:11840
Create data:    172.17.0.3:11840
Create data:    172.17.0.4:11840
```



## 启动一个 MySQL 实例容器

```lang-bash
$ docker run -it -d -p 3306:3306 --name mysql sequoiadb/sequoiasql-mysql:latest
```



## 查看启动容器的 ID

```lang-bash
$ docker ps -a | awk '{print $NF}';
```

输出结果为包括 MySQL 实例在内的所有容器名：

```lang-bash
NAMES
mysql
sdb_data3
sdb_data2
sdb_data1
coord_catalog
```



## 查看容器 IP 地址

```lang-bash
$ docker inspect mysql | grep IPAddress | awk 'NR==2 {print $0}'
```

输出结果为 MySQL 实例的 IP 地址：

```lang-bash
            "IPAddress": "172.17.0.6",
```



## 将 MySQL 实例注册入协调节点

```lang-bash
$ docker exec mysql "/init.sh" --port=3306 --coord='172.17.0.2:11810'
```

输出结果为：

```lang-bash
Creating SequoiaSQL instance: MySQLInstance
Modify configuration file and restart the instance: MySQLInstance
Restarting instance: MySQLInstance
Opening remote access to user root
Restarting instance: MySQLInstance
Instance MySQLInstance is created on port 3306, default user is root
```



## 本地登陆 MySQL 测试

```lang-bash
$ mysql -h 127.0.0.1 -P 3306 -u root
```

可以得到 MySQL 连接成功的输出：

```lang-sql
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 2
Server version: 5.7.25 Source distribution

Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.
```

用户可以使用 MySQL 命令创建数据库与表：

```lang-sql
mysql> create database sample;
Query OK, 1 row affected (0.00 sec)

mysql> use sample;
Database changed
mysql> create table t1 (c1 int);
Query OK, 0 rows affected (0.59 sec)

mysql> show table status;
+------+-----------+---------+------------+------+----------------+-------------+-----------------+--------------+-----------+----------------+-------------+-------------+------------+-------------+----------+----------------+---------+
| Name | Engine    | Version | Row_format | Rows | Avg_row_length | Data_length | Max_data_length | Index_length | Data_free | Auto_increment | Create_time | Update_time | Check_time | Collation   | Checksum | Create_options | Comment |
+------+-----------+---------+------------+------+----------------+-------------+-----------------+--------------+-----------+----------------+-------------+-------------+------------+-------------+----------+----------------+---------+
| t1   | SequoiaDB |      10 | Fixed      |    0 |              0 |           0 |   8796093022208 |       131072 |         0 |           NULL | NULL        | NULL        | NULL       | utf8mb4_bin |     NULL |                |         |
+------+-----------+---------+------------+------+----------------+-------------+-----------------+--------------+-----------+----------------+-------------+-------------+------------+-------------+----------+----------------+---------+
1 row in set (0.16 sec)
```



## 重置镜像

为方便用户重置已经创建了数据库节点的容器，用户可以使用 cleanup.sh 脚本进行本地容器的重置。

```lang-bash
$ docker exec mysql /cleanup.sh
$ docker exec coord_catalog /cleanup.sh
$ docker exec sdb_data1 /cleanup.sh
$ docker exec sdb_data2 /cleanup.sh
$ docker exec sdb_data3 /cleanup.sh
```


> **Note：**  
> 该集群仅为测试使用，不可直接应用于生产环境。