# SequoiaDB 监控指标
    
本文档将介绍 SequoiaDB 巨杉数据库中 MySQL 实例的相关监控。

## MySQL 监控统计信息相关

- 通过 MySQL 命令查看

    ```lang-sql
    mysql> show global status like 'sdb_share%';
    ```
    
- 变量说明

**sdb_share_inserted**

总共将表统计信息加入到全局缓存信息中的次数。

**sdb_share_updated**

总共将表统计信息进行更新的次数。

**sdb_share_deleted**

总共将表统计信息从全局缓存信息中踢出的次数。