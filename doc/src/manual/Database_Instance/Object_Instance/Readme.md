[^_^]:
    文件系统实例概述
    作者：赵玉静
    时间：20190820
    评审意见
    王涛：
    许建辉：
    市场部：20191107


SequoiaDB 除了支持创建关系型数据库实例和 JSON 实例外，同样支持创建 S3 与 POSIX 文件系统的对象存储实例。

其中，S3 对象存储实例适用于对象存储类的联机业务与归档类场景，SequoiaDB 与 S3 保持 100% 兼容。SequoiaS3 系统实现通过 AWS S3 接口访问 SequoiaDB 的能力。

POSIX 文件系统适用于使用传统文件系统向分布式环境迁移的业务场景，SequoiaDB 与标准 Ext3/XFS 等基本保持兼容。SequoiaFS 文件系统是基于 FUSE 在 Linux 系统下实现的一套文件系统，支持通用的文件操作 API。

本章将介绍 S3 对象存储和 POSIX 文件系统实例的操作与开发。

## S3 对象存储实例

**操作**

 - [安装与配置][s3_setup]
 - [Rest 接口][restapi]
 - [Java 接口][javaapi]

**开发**

 - [S3 驱动下载][s3_engine_download]
 - [通过 s3cmd 进行连接][s3_connection]
 - [Java 程序样例][s3_java_sample]
 - [C++ 程序样例][C++_sample]

## POSIX 文件系统实例

**操作**

 - [安装部署][install_deploy]
 - [SequoiaFS 使用][mount]
 - [配置管理][sequoiafsconfig]
 - [SequoiaFS 启停][sequoiafsbash]


**开发**

 - [数据设计][datadefine]
 - [API][fileapi]




[^_^]:
     本文使用的所有链接及引用
[s3_setup]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/setup.md
[restapi]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/restapi.md
[javaapi]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/javaapi.md
[s3_engine_download]:manual/Database_Instance/Object_Instance/S3_Instance/Development/engine_download.md
[s3_connection]:manual/Database_Instance/Object_Instance/S3_Instance/Development/connection.md
[s3_java_sample]:manual/Database_Instance/Object_Instance/S3_Instance/Development/java_sample.md
[C++_sample]:manual/Database_Instance/Object_Instance/S3_Instance/Development/C++_sample.md

[datadefine]:manual/Database_Instance/Object_Instance/File_Instance/Development/datadefine.md
[install_deploy]:manual/Database_Instance/Object_Instance/File_Instance/install_deploy.md
[fileapi]:manual/Database_Instance/Object_Instance/File_Instance/Development/fileapi.md
[mount]:manual/Database_Instance/Object_Instance/File_Instance/Operation/mount.md
[sequoiafsconfig]:manual/Database_Instance/Object_Instance/File_Instance/Management/sequoiafsconfig.md
[sequoiafsbash]:manual/Database_Instance/Object_Instance/File_Instance/Management/sequoiafsbash.md