[^_^]:
    产品概述
    作者：王涛
    时间：20190211
    评审意见
    王涛：初稿完成，时间：20190211
    许建辉：时间：20190214
    市场部：时间：20190226


SequoiaDB 巨杉数据库是一款开源的金融级分布式关系型数据库，主要面对高并发联机交易型场景提供高性能、可靠稳定以及无限水平扩展的数据库服务。

用户可以在 SequoiaDB 巨杉数据库中创建多种类型的数据库实例，以满足上层不同应用程序各自的需求。

SequoiaDB 巨杉数据库支持 MySQL、MariaDB、PostgreSQL 和 SparkSQL 四种关系型数据库实例、类 MongoDB 的 JSON 文档类数据库实例、以及 S3 对象存储与 POSIX 文件系统的非结构化数据实例。

关键特性
----
SequoiaDB 巨杉数据库可以为用户带来如下价值：
+ 完全兼容传统关系型数据，数据分片对应用程序完全透明
+ 高性能与无限水平弹性扩展能力
+ 分布式事务与 ACID 能力
+ 同时支持结构化、半结构化与非结构化数据
+ 金融级安全特性，多数据中心间容灾做到 RPO=0
+ HTAP 混合负载，同时运行联机交易与批处理任务且互不干扰
+ 多租户能力，云环境下支持多种级别的物理与逻辑隔离

用户案例
----
当前已经有超过 [50 家银行机构与上百家企业级用户][userlist]在生产环境大规模使用 SequoiaDB 巨杉数据库取代传统数据库。

使用场景
----
SequoiaDB 巨杉数据库拥有三大类应用场景，用户可参考[应用场景][usecase]页面获得更多信息。
+ [联机交易][onlinetransaction]
+ [数据中台][mid-end]
+ [内容管理][contentmanagement]


[^_^]:
    TODO:该页面需要调整

[userlist]:http://solution.sequoiadb.com/cn/
[usecase]:manual/SequoiaDB_Intro/usage.md
[onlinetransaction]:manual/SequoiaDB_Intro/usage.md
[mid-end]:manual/SequoiaDB_Intro/usage.md
[contentmanagement]:manual/SequoiaDB_Intro/usage.md