[^_^]:
    数据库实例概述
    作者：
    时间：
    评审意见
    王涛：  时间：
    许建辉：时间：
    市场部：时间：


SequoiaDB 巨杉数据库采用计算存储分离架构。数据库底层以支持分布式事务能力的存储节点构建可横向扩展的存储集群，上层通过创建多实例的方式提供 MySQL、MariaDB、PostgreSQL、以及 SparkSQL 的支持。同时，除了支持结构化 SQL 实例以外，SequoiaDB 巨杉数据库还支持创建 JSON、S3对象存储、以及 Posix 文件系统的非结构化实例。

![structure][structure]

SequoiaDB 巨杉数据库的分布式架构一方面可以提供针对数据表的无限横向水平扩张，另一方面在计算层通过提供不同类型数据库实例的方式，100% 兼容 MySQL、PostgreSQL 与 SparkSQL 协议与语法，原生支持跨表跨节点分布式事务能力，应用程序基本可以在零改动的基础上进行数据库迁移。

除了结构化数据外，SequoiaDB 巨杉数据库可以在同一集群支持包括 JSON、S3 对象存储以及 Posix 文件系统在内的非结构化数据，使整个数据库面向上层的微服务架构应用提供了完整的数据服务资源池。

本文档将主要介绍 SequoiaDB 所支持的三类数据库实例的操作和开发：

- 关系型数据库实例
  - [MySQL 实例][mysql]
  - [MiraDB 实例][mariadb]
  - [PostgreSQL 实例][pgsql]
  - [SparkSQL 实例][sparksql]
- [JSON 实例][json]
- 对象存储实例 
  - [S3 对象存储实例][s3]
  - [Posix 文件系统实例][posix]


[structure]:images/Database_Instance/structure.png
[mysql]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Readme.md
[pgsql]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Readme.md
[sparksql]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Readme.md
[json]:manual/Database_Instance/Json_Instance/Readme.md
[s3]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/Readme.md
[posix]:manual/Database_Instance/Object_Instance/File_Instance/Readme.md
[mariadb]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Readme.md