
用户可以在 SequoiaDB 巨杉数据库中创建多种类型的数据库实例，以满足上层不同应用程序的需求。数据库实例的访问与使用方式和传统关系型数据库完全兼容，同时其底层所使用的数据从逻辑上完全独立，每个实例拥有自己独立的权限管理、数据管控、甚至可以选择部署在独立的硬件环境或共享设备中。

SequoiaDB 支持创建 MySQL、MariaDB、PostgreSQL 和 SparkSQL 四种关系型数据库实例，并且完全兼容 MySQL、MariaDB、PostgreSQL 和 SparkSQL。用户可以使用 SQL 语句访问 SequoiaDB
，完成对数据的增、删、查、改操作以及其他语法操作。

本章将主要介绍四种关系型数据库实例的操作与开发，帮助用户学习如何创建不同类型的数据库实例，并实现应用程序从传统数据库进行无缝迁移，大幅度降低应用程序开发者的学习成本。

||MySQL实例|PostgreSQL实例|SparkSQL实例|MariaDB实例|
|----|----|----|----|----| mariadb
|操作|[安装部署][setup]<br>[使用][usage]<br>[配置][config]<br>[高可用][mysql_ha]<br>[实例组][mysql_instance_group]<br>[实例管理工具][sdb_mysql_ctl]<br>[分区][partition]<br>[数据类型映射表][data_type]<br>[注意事项][attention]<br>[错误码][error_code]<br>[升级][upgrade]<br>[卸载][uninstall]|[安装部署][install]<br>[连接][connect]<br>[SQL 实例与 JSON 对象映射表][mapping]<br>[卸载][un]|[安装部署][deploy]<br>[连接][spark_conn]<br>[使用][use]|[安装部署][deploy_maria]<br>[使用][connection]<br>[配置][maria_config]<br>[高可用][maria_ha]<br>[实例组][maria_instance_group]<br>[实例管理工具][sdb_maria_ctl]<br>[数据类型映射表][maria_datatype]<br>[注意事项][maria_atten]<br>[错误码][maria_error]<br>[升级][upgrade_maria]<br>[卸载][maria_uninstall]|
|开发|[MySQL 驱动下载][engine_download]<br>[JDBC 驱动][JDBC]<br>[ODBC 驱动][ODBC]|[PostgreSQL 驱动下载][engine]<br>[JDBC][JD]<br>[ODBC][OD]|[Spark 驱动下载][download]<br>[JDBC][BC]|


[^_^]:
     本文使用的所有引用及链接
[sdb_mysql_ctl]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/sdb_mysql_ctl.md
[partition]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Operation/partition.md
[setup]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/install_deploy.md
[mysql_ha]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/ha.md
[mysql_instance_group]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/instance_group.md
[upgrade]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/upgrade.md
[usage]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Operation/connection.md
[config]: manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/config.md
[data_type]:manual/Database_Instance/Relational_Instance/MySQL_Instance/data_type.md
[attention]:manual/Database_Instance/Relational_Instance/MySQL_Instance/attention.md
[error_code]:manual/Database_Instance/Relational_Instance/MySQL_Instance/error_code.md
[uninstall]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/uninstall.md
[engine_download]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Development/engine_download.md
[JDBC]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Development/JDBC.md
[ODBC]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Development/ODBC.md

[install]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/install_deploy.md
[connect]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/connection.md
[mapping]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/SQL_to_Sequoiadb_mapping.md
[un]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/uninstall.md
[engine]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/engine_download.md
[JD]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/JDBC.md
[OD]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/ODBC.md

[deploy]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Operation/setup.md
[spark_conn]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Operation/connection.md
[use]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Operation/usage.md
[download]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Development/engine_download.md
[BC]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Development/JDBC.md

[deploy_maria]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/install_deploy.md
[maria_ha]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/ha.md
[maria_instance_group]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/instance_group.md
[upgrade_maria]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/upgrade.md
[connection]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Operation/connection.md
[maria_atten]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/attention.md
[maria_config]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/config.md
[maria_datatype]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/data_type.md
[maria_error]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/error_code.md
[sdb_maria_ctl]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/sdb_maria_ctl.md
[maria_uninstall]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/uninstall.md