SequoiaDB 巨杉数据库是一款金融级分布式数据库，产品引擎采用原生分布式架构，协议级兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用SequoiaDB](quickstart.md)

##SequoiaDB version 3.2.8 版本说明##

**接口变更：**

- 存储引擎增加配置项"indexcoveron"：是否开启"Index Cover"能力，默认为开启
- MySQL引擎创建索引开启"Not Array"属性
- 新增对 MySQL 5.7.32 的支持

**主要特性：**

- 索引支持"Not Array"属性，开启"Not Array"属性时该索引对应的所有列不允许为数组
- 提供审计日志服务，实现审计日志集中管理、查看
- 支持多数据源功能，可以在该集群中访问映射的数据源的集合空间和集合：
  - 创建、修改、删除、查看数据源
  - 支持对数据源的访问进行权限控制
  - 支持对数据源的访问进行容错
  - 支持对数据源的集合空间进行同名/不同名映射
  - 支持对数据源的集合进行同名/不同名映射
  - 支持对映射的数据源的集合进行"Insert"、"Delete"、"Update"、"Query"的记录操作
  - 支持对映射的数据源的集合进行"Count"、"Truncate"的元数据操作
  - 支持对映射的数据源的集合进行"Create"、"Read"、"Write"、"Delete"的大对象操作

- SAC增加操作日志管理功能，可以查看详细操作信息

**性能优化：**

- 支持"Index Cover"能力：查询操作所有字段都命中索引，可以直接使用索引键值，而不需要读取记录，提升性能
- 在高并发访问时，降低编目节点访问压力，提升整体性能
- 在数据查询操作中减少一次"GetMore"消息交互，提升查询性能

**工具优化：**

- 日志查看工具(sdbdpsdump)增加过滤事务id的能力
- MySQL实例管理工具(sdb_mysql_ctl)增加参数"sequoiadb_lock_wait_timeout"、"sequoiadb_use_rollback_segments"等的配置
- 数据导入工具(sdbimprt)提供字符串截断功能
- 灾备工具去除对系统用户名和密码的依赖

**解决重要Bug：**

- 修复 数据节点升主时回滚事务可能与新发起自动提交事务卡住的问题
- 修复 PostgreSQL引擎查询索引失败后，释放BSON内存时发生coredump的问题
- 修复 TRACE跟踪过程中无法停止节点的问题
- 修复 特定场景下参数"tranreplsize"配置不生效的问题


##SequoiaDB version 3.2.7 版本说明##

**接口变更：**

NA

**主要特性：**

NA

**性能优化：**

- 主表上的查询只匹配到一个子表时，减少内部排序逻辑，提升查询性能

**工具优化：**

NA

**解决重要Bug：**

- 修复 MySQL 中 count 下压时，字段对大小写敏感的问题
- 修复 MySQL 在某些场景下在 session 级别对事务开关、隔离级别进行设置可能失效的问题
- 完善 PostgreSQL 中带时区查询时对时区的处理
- 修复 SAC 中扩容、减容等场景下的鉴权问题


##SequoiaDB version 3.2.6 版本说明##

**接口变更：**

- MySQL 引擎
  - 配置项 *sequoiadb_use_transaction* 新增在 session 级别设置的能力
  - 新增配置项 *sequoiadb_use_rollback_segments、sequoiadb_optimizer_options、sequoiadb_lock_wait_timeout*
- SequoiaDB
  - 新增配置项 *servicemask*
  - Java/Python/PHP/C# 驱动增加快照类型：SDB_SNAP_QUERIES、SDB_SNAP_LATCHWAITS、SDB_SNAP_LOCKWAITS
  - REST 中 insert 接口支持 flag 参数

**主要特性：**

- MySQL 引擎
  - 新增对 MySQL 5.7.31 的支持
  - 支持修改自动生成列
  - 支持通过密码文件与 SequoiaDB 建立连接

- SequoiaDB
  - 通过配置项 servicemask 关闭 SequoiaDB 节点指定服务端口
  - 集合增加 getDetail() 接口，用于获取集合快照信息

**性能优化：**

- MySQL 支持部分条件、order by、group by 及 limit 下压，提升查询性能
- MySQL 元数据同步工具改为多线程同步，提升同步性能
- 优化 BSON 中 int 与 long 类型数据的比较性能
- 优化事务中索引查询的锁获取机制，提升 count() 性能

**工具优化：**

- MySQL 元数据同步工具完善对存储过程的同步及对注释的处理
- sdb shell 支持通过加密文件方式输入密码
- sdb shell 历史命令中不记录与密码相关的命令
- 导入工具支持对索引键重复的记录进行替换
- 导入、导出工具支持字符分隔符为空
- 导入工具内存分配机制优化，提升包含多层嵌套 json 的记录的性能
- SAC 支持新增、查看、删除自增字段

**解决重要Bug：**

- OpenSSL 库升级到 OpenSSL-1.1.1g 版本，解决老版本安全漏洞问题
- 修改 ListLobs 指定 OID 查询无效的问题
- 解决特定场景下集合压缩器扫描数据导致 CPU 占用高的问题

##SequoiaDB version 3.2.5 版本说明##

**接口变更：**

- MySQL 引擎
  - 支持在 MySQL 中指定表的压缩类型

- SequoiaDB
  - REST 接口增加自增字段操作命令 create autoIncrement 和 drop autoIncrement
  - 节点健康快照中增加 StackSize 字段

**主要特性：**

- 新增对 MySQL 5.7.28 的支持
- MySQL 完善对 alter table 命令的支持
- 提供分区组容错熔断机制，通过配置开启错误和风险智能检测，并提供“熔断”、“半容错”和“全容错”三种容错级别，实现高可用
- 支持在更新操作符中，使用一个字段的值去更新其它字段
- 新增交互式和加密文件两种密码输入方式
- 全文检索适配 ElasticSearch 6.8.5 版本

**性能优化：**

- MySQL 支持下压使用一个字段更新其它字段的操作，提升更新性能
- MySQL 实现索引逆序读，提升排序性能
- SequoiaDB 节点启动性能优化，提升启动速度
- 事务老版本清理性能优化，提升清理性能
- 优化分区命中算法，提升查询性能


**工具优化：**

- sdbpasswd 工具完善和优化

**解决重要Bug：**

- 修改升级时 DataCommitLSN 在某些场景下为 -1 的问题

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
