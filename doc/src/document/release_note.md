SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，产品引擎采用原生分布式架构，100%兼容 MySQL 语法和协议，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。
本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用SequoiaDB](quickstart.md)

##SequoiaDB version 3.2.6 版本说明##

**接口变更：**

- MySQL 引擎
  - 配置项 *sequoiadb_use_transaction* 新增在 session 级别设置的能力
  - 新增配置项 *sequoiadb_use_rollback_segments*、*sequoiadb_optimizer_options*
- SequoiaDB
  - 新增配置项：*servicemask*
  - Java/Python/PHP/C# 驱动增加快照类型：SDB_SNAP_QUERIES、SDB_SNAP_LATCHWAITS、SDB_SNAP_LOCKWAITS
  - REST 中 insert 接口支持 flag 参数
  - SequoiaFS 增加区域的创建、查询、删除等接口

**主要特性：**

- MySQL/MariaDB 支持通过密码文件与 SequoiaDB 建立连接
- 通过配置项 servicemask 关闭 SequoiaDB 节点指定服务端口
- 集合增加 getDetail() 接口，用于获取集合快照信息
- 支持在更新操作符中，使用一个字段的值去更新其它字段
- 节点健康快照中增加 StackSize 字段
- 提供 SequoiaFS 的启动、停止及查询脚本

**性能优化：**

- 优化 BSON 中 int 与 long 类型数据的比较性能
- 优化事务下索引查询时的锁获取机制，提升 count() 性能

**工具优化：**

- sdb shell 支持通过加密文件方式输入密码
- sdb shell 历史命令中不记录与密码相关的命令
- 导入工具支持对索引键重复的记录进行替换
- 导入、导出工具支持字符分隔符为空
- 导入工具内存分配机制优化，提升包含多层嵌套 json 的记录的性能
- sdbpasswd 工具完善和优化
- SAC 支持新增、查看、删除自增字段

**解决重要Bug：**

- OpenSSL 库升级到 OpenSSL-1.1.1g 版本，解决老版本安全漏洞问题
- 修改 ListLobs 指定 OID 查询无效的问题
- 修改 SequoiaFS 挂载目录的权限设置问题
- 修改集合上 truncate 与 dropIndex 并发重放时的报错问题
- 解决特定场景下集合压缩器扫描数据导致 CPU 占用高的问题


##SequoiaDB version 3.2.4 版本说明##

**接口变更：**

- MySQL引擎
  - 配置项 *sequoiadb_use_partition* 更名为 *sequoiadb_auto_partition* ；
  - 废弃配置项 *sequoiadb_optimizer_select_count* ；
  - 新增配置项 *sequoiadb_use_transaction*、 *sequoiadb_optimizer_options*；
- REST接口支持返回标准JSON格式；
- 提供标准S3兼容的对象访问接口，实现“桶”和“对象”操作以及“多版本控制”；
- 提供 SQL 化监控视图对数据库进行监控，可以使用SQL的各种能力灵活筛选和组合监控数据，提升监控的易用性；

**主要特性：**

- MySQL引擎
  - 支持无事务模式，能够实现批量非事务的高性能操作；
  - 完善表、列、主键和索引的修改操作；
  - 支持多实例元数据实时同步，提供高可用能力；
  - Update/Delete/Count/Autocommit下推优化，提升性能；
  - 支持 Insert ... on duplicate key update ... 语法；
- Insert/Update/Delete等支持返回记录数和详细错误信息；
- 插入数据支持重复键替代( insert ... on duplicate replace )；
- 访问计划实现自动过期清理，以及对 $in 操作进行参数化和缓存；
- 全文索引支持字符串数组，以及 $or 和 $not 操作；
- 索引支持 not null 约束；
- 命令位置参数支持 InstanceID ；
- 大对象存储支持按时间序进行垂直分区，提升对大对象的存取和管理能力，可以快速按时间进行归档和清理；
- 大对象List操作支持过滤条件和精准匹配；
- 重选举支持指定节点；
- 复制日志支持开启全量模式和时间字段，可以通过工具进行增量数据抽取；
- 多唯一索引的集合支持副本节点并发数据同步和重放；

**性能优化：**

- 对snapshot transaction进行性能优化，减少对业务操作的影响；
- 全文索引count以及访问性能优化；同时优化连接为共享连接，减少ES引擎内存开销；
- 实现多层级内存池模型，提升访问性能；并提供在线内存监控和离线分析能力；

**工具优化：**

- SHELL的File对象增加truncate接口；
- SHELL增加IniFile对象；
- sdbreplay支持按周期或指定时间将增量数据输出到文件；
- SAC
  - 创建集合支持自增字段；
  - 支持创建全文索引和全文索引操作；
  - 支持"数据库实例"配置修改和同步；

**解决重要Bug：**

NA

##SequoiaDB version 3.2.3 版本说明##

**接口变更：**

- 集合快照增加访问信息；
- 访问计划默认级别调整为3；
- 通信默认开启多连接多线程模型；
- 事务隔离级别默认调整为RC；

**主要特性：**

- 支持多连接多线程通信模型；
- 完善全局事务一致性；
- SAC支持“发现数据库实例”和“数据库实例同步”功能；

**性能优化：**

- 提供基于线程的内存管理机制，内存性能提升10%以上；
- MySQL支持选择符下推，查询性能提升10%以上；
- MySQL支持 Auto-Commit 下推，精准查询性能提升40%以上；

**工具优化：**

- TRACE支持按会话、会话类型以及函数进行过滤；同时优化格式化输出；
- SDBSHELL 提供 NumberDecimal 对象，完善快捷键和历史命令搜索功能；

**解决重要Bug：**

NA

##SequoiaDB version 3.2 版本说明##

**接口变更：**

- 变更：事务快照支持查看锁的等待或持有时间。
- 新增：新增配置参数“translockwait”、“transautocommit”、“transautorollback”、“transuserbs”、“preferedstrict”。
- 新增：新加配置更新、删除接口。

**主要特性：**

- 完善事务隔离级别，支持RU(Read Uncommited)，RC(Read Committed)和RS(Read Stability)。
- 支持会话级 事务隔离级别、事务超时、autocommit、autorollback 等。
- 支持事务 autocommit 能力。
- 会话读写分离属性支持严格模式，当为严格模式时只会在指定节点上进行读取。
- 集合数据在线切分支持事务。
- 支持用户级审计日志。
- 查询支持 Select for update。
- 事务快照支持查看锁的等待或持有时间。
- 记录支持自增序列。
- 提供全局配置修改、删除能力。
- 提供完整S3接口的对象管理能力。
- 完善集合空间改名、集合改名能力。
- SAC提供配置在线修改能力。
- SAC提供重启服务能力。

**性能优化：**

- 优化节点启动时事务日志的加载性能。
- 优化记录锁性能。
- 优化内存管理性能。

**工具优化：**

- 工具支持安全密码和交互式密码能力。

**解决重要Bug：**

NA

**注意事项：**

- 对于使用了全文检索的环境，升级过程中要同时升级并重启适配器进程。由于 3.0 之后的版本对全文索引相关机制进行了调整优化，如果升级前版本为 3.0，在升级过程中，需要在停数据节点前，先将所有适配器停止，然后再进行节点升级，或者在升级前将全文索引删除，在升级完成后重建。
