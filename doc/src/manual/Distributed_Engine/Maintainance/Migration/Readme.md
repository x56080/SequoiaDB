[^_^]:
    迁移

为了方便与传统数据库在数据层进行对接，SequoiaDB 巨杉数据库提供多种数据导入及导出的方法，用户可以根据自身需求选择最合适的方案完成数据迁移。

- **数据导入**

    SequoiaDB 支持通过 sdbimprt 工具和第三方数据复制的方式将数据导入至集群。针对第三方数据复制的导入方式，SequoiaDB 支持使用 Oracle 官方迁移工具、第三方迁移工具等方式，从 DB2 和 Oracle 中实时同步数据至 SequoiaDB。同时支持基于 MySQL 的 binlog Replication 机制，将 MySQL 中的数据实时复制至 SequoiaDB。

- **数据导出**

    SequoiaDB 支持使用 sdbexprt 工具将集群的数据导出到 CSV 或 JSON 数据存储文件中。

通过本章文档，用户可以了解 SequoiaDB 数据迁移的相关步骤。主要内容如下：

- [使用 sdbimprt 导入数据][import]
- [实时第三方数据复制][third_party_realtime]
- [使用 sdbexprt 导出数据][export]



[^_^]:
     本文使用的所有引用和链接
[import]:manual/Distributed_Engine/Maintainance/Migration/import.md
[third_party_realtime]:manual/Distributed_Engine/Maintainance/Migration/third_party_realtime.md
[export]:manual/Distributed_Engine/Maintainance/Migration/export.md
