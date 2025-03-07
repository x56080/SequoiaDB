SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，产品引擎采用原生分布式架构，100% 兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用 SequoiaDB][quickstart]

##注意事项##
- 从 3.4.4/3.6/5.0.3 及早期版本滚动升级到 3.4.5/3.6.1/5.0.4 及之后的版本时，从 SQL 引擎执行的 INSERT 操作会存在失败。因此滚动升级的过程中需保证优先完成存储引擎的升级，然后再进行 MySQL/MariaDB 实例的升级。
- 从 3.4.4/3.6/5.0.3 及早期版本升级到 3.4.5/3.6.1/5.0.4 及之后的版本，如果集群会扩展为 X86 和 ARM 架构混合部署，则在升级前版本上创建的、使用 double 类型字段作为 hash 分区键的集合，需要进行重建，否则可能会出现数据无法正确访问的问题。可通过查询 SDB_SNAP_CATALOG 快照，根据集合使用的 hash 算法版本号（InternalV 字段）判断，对于该版本号小于 4 的集合需要进行处理。

##SequoiaDB version 5.8.4 版本说明##

**接口变更：**

- 存储引擎
  - LOB运维命令优化：
     - SdbCollection.listLobs() 接口支持 cond/sel/hint/limit/skip 参数，返回结果增加分片数据实际大小和分片所在组信息；
	 - 增加 SdbCollection.lobCount() 接口，统计当前集合符合条件的 LOB 总数；
	 - SdbCollection.getLobDetail() 接口，返回结果增加分片在数据组中的分布信息；
	 - 增加 SdbCollection.lobExplain() 接口，获取大对象在集合中分片位置信息的执行计划；
	 - SdbCollection.getDetail() 支持合并结果集；
  - forceStepup 增加选举控制参数：
	 - WaitSeconds，升主等待超时时间；
	 - Enforced，该参数为 true 时，即使当前不符合 step up 的条件，也进行强制升主；
  - 重选举支持设置选举级别，阻塞不同的写操作：
     - Level 1: 等待当前写操作结束，并阻塞后续写操作；
     - Level 2: 等待写游标(大对象操作)结束；
     - Level 3: 等待事务结束；
  - 新增节点级别参数控制集合replsize和consistencystrategy属性，集合 ReplSize 默认值改为 2；

- SQL 引擎
  - 新增 preload-sdb-stats 选项提供实例启动时自动加载统计信息功能

**主要特性：**

- 存储引擎
  - 优化 Deleting 记录的回收机制，减少对在线业务性能影响；

- SQL 引擎
  - MySQL 升级至 5.7.44；
  - 提供实例启动时自动加载统计信息功能；

**性能优化：**

- 存储引擎
  - 残留和空 CS 自动清理优化；
  - 优化索引统计信息子表采样合并机制，提升访问计划评估准备性和稳定性；
  - 特定场景下（当数据节点集合空间只有一个集合时），dropCL 优化为 dropCS，提升性能；
  - 优化创建索引数据扫描和排序的加锁机制，减少对写入操作的阻塞时间；
  - 加载集合空间内存优化；
  - 优化节点启动/停止流程，加速节点启动/停止；
  - 优化索引内存并增加索引缓存清理；
  - 增加 meta 文件定时刷盘机制，加速节点停止；

- SQL 引擎
  - 优化带 order by 和 limit 子句的查询的访问计划；

**工具优化：**

- 存储引擎
  - 索引升级工具性能优化；
  - 快速部署工具支持 MariaDB 快速部署；
  - sdbdmsdump 工具优化改进，提升性能 50%；

**解决重要Bug：**

- 存储引擎
  - 修复大并发执行快照采集导致协调节点主线程消息队列阻塞的问题；
  - 修复 createCL 使用 RefFrom 复制特定选项集合的功能问题；
  - 修复 CS 文件 create/drop/truncate 产生 DEADSYNC 和 SLOWNODE 的问题；
  - 修复编目节点发送消息报 -85 错误导致所有协调节点连接卡住的问题；
  - 修复集合空间 DataCommitted 标记不正确，导致发生非预期的全量同步的问题；
  - 修复 C++ 驱动 cl.query() 与 cl.interrupt() 并发执行偶现卡死的问题；
  - 修复 ARM 平台上数值转换由于编译器优化导致结果不正确的问题；
  - 修复索引平衡导致节点 coredump 的问题；
  - 修复开启容错，备节点异常时，Lob 写操作无法降级处理的问题；
  - 修复创建索引主备索引 UniqueID 不一致的问题，该问题对索引使用无影响；

- SQL 引擎
  - 修复索引读不能正确处理夏令时的问题；
  - 修复索引统计信息未被正确清除的问题；
  - 修复 DDL 后统计信息可能重复加载的问题；

##SequoiaDB version 5.8.3 版本说明##

**接口变更：**

- 存储引擎
  - sdb shell 新增 runtime-size 参数，动态调整 sdb shell 最大运行内存
  - sdbcm 新增 ValidTimeThreshold 参数，设置异常节点自动重启间隔时间。如果在间隔时间内，sdbcm 自动重启异常节点次数超过 RestartCount 参数的限制，则 sdbcm 不会再重启该异常节点

**主要特性：**

- 存储引擎
  - 慢查询新增监控指标:
     - QueryCataTime：查询编目耗时，单位为毫秒
     - BlockTime：操作被阻塞的时间，单位为毫秒
     - QueryCataCount：查询编目的次数
     - BlockType：阻塞事件类型(如果为空则不显示该字段)，有以下事件类型：FreezingWindow, DMSBlock, WaitPrimary, WaitTransRollback, WaitRelect, SyncControl, WaitFusing，NoLogSpace
     - FileOPTime：节点在文件层操作的耗时（该指标仅在数据节点显示），单位为毫秒
     - DispatchTimeSpent：消息分发花费时间，单位为毫秒
     - SortTime：数据排序花费时间，单位为毫秒
     - HashCode：查询语句的哈希标识，相同哈希标识对应同类型的查询语句 （仅数据节点）
     - LogOPTime：读写同步日志的耗时，单位为毫秒（仅数据节点）
     - TransLockWaitCount：锁等待次数（仅数据节点）
     - LatchWaitCount：闩锁等待次数（仅数据节点）
  - 慢查询参数 mongroupmask 从开改为关时，会立即清理掉历史的监控信息，优化为保留 5 分钟再清理；
  - createCL 支持指定创建多个数据组；
  - createCL/createCS 支持克降模式；
  - 健康快照支持显示节点切主信息；

**性能优化：**

- 存储引擎
  - 优化查询快照和事务快照性能
  - 优化索引统计信息子表采样率

- SQL 引擎
  - 增加统计信息缓存，优化获取统计信息性能

**工具优化：**

- 存储引擎
  - 修复容灾脚本在多个安装路径各不相同的机器上执行失败的问题

**解决重要Bug：**

- 存储引擎
  - 修复查询快照死锁问题
  - 修复 transactionon 为 false 时，执行事务相关快照导致节点 coredump 问题
  - 修复访问计划索引评估出错的问题
  - 修复事务操作中会话快照和查询快照 LastOpInfo 字段值为空的问题

- SQL 引擎
  - 修复对不存在的库进行授权，实例组同步报错的问题
  - 修复 refresh 操作无法获取全部索引统计信息的问题
  - 修复对不存在的库执行 analyze 操作导致回放线程卡住的问题

##SequoiaDB version 5.8.2 版本说明##

**接口变更：**

- 存储引擎
  - 新增节点级配置参数 nettimeout

**主要特性：**

- 存储引擎
  - SDB 支持编目节点消息防阻塞机制

**性能优化：**

- 存储引擎
  - $range 查询性能优化

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复数据切分过程中，备节点 LSN 差距越来越大，导致全量同步的问题
  - 修复大规模停止线程场景下，新连接查询操作卡住的问题
  - 修复 upsert 操作出现唯一键冲突时，日志中打印过多 -38 ERROR 日志的问题

- SQL 引擎
  - 修复 MySQL BKA join 缺少结果集的问题

##SequoiaDB version 5.8.1 版本说明##

**接口变更：**

NA

**主要特性：**

NA

**性能优化：**

NA

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复锁升级场景下记录删除失败的问题
  - 修复空子表影响主表查询计划缓存，导致慢查询的问题
  - 修复数据页多但记录少的场景下，通过唯一键访问走表扫描的问题
  - 修复事务操作执行过程中节点崩溃，恢复后事务状态不一致的问题

##SequoiaDB version 5.8 版本说明##

**接口变更：**

- 存储引擎
  - sdb shell 新增 memtrim() 接口

**主要特性：**
- 存储引擎
  - 支持精细化容灾
    - 节点支持设置 location 属性
    - 基于 location 的集群管理优化
    - 支持容灾 critical 模式
    - 支持容灾 maintenance 模式
  - 升级权限管理功能，支持基于角色的访问控制
  - 优化内存管理，提供内存回收的能力

**性能优化：**

NA

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复切主导致的读事务残留的问题
  - 修复创建索引中断导致主备节点 IndexCommitLsn 不一致的问题

- SQL 引擎
  - 修复 SQL 日志中包含敏感信息的问题
  - 修复 SQL 隔离级别为 SERIALIZABLE 时导致 crash 的问题

##SequoiaDB version 5.6.2 版本说明##

**接口变更：**

- SQL 引擎
  - sdb_sql_ctl 工具新增 load-stats 参数，用于启动时加载统计信息

**主要特性：**

- 存储引擎
  - 提供数据恢复工具 sdbrevert

- SQL 引擎
  - MySQL 支持 v5.7.42
  - SequoiaDB 存储引擎的表支持查看建表时间
  - 实例组集合空间支持指定数据域

**性能优化：**

- SQL 引擎
  - BKA 查询减少 SequoiaDB 不必要的 sort 操作
  - 统计信息不准确时，优先使用命中更多字段的索引

**工具优化：**

- SQL 引擎
  - 实例组新建实例，全量同步时源实例不加锁
  - 优化实例组存在大量实例场景下的同步

**解决重要Bug：**

- 存储引擎
  - 修复复合索引选择错误的问题
  - 修复快速部署工具部署端口相同的节点时不报错的问题
  - 修复源复制组数据 100% 切分至目标组后，源复制组集合空间仍然存在的问题

- SQL 引擎
  - 修复 SQL 的 autocommit 可能被不正确地下压到 SequoiaDB 的问题
  - 修复 direct_update(direct_delete) 执行 index merge 访问计划时可能有错误结果的问题

##SequoiaDB version 5.6.1 版本说明##

**接口变更：**

- SQL 引擎
  - 新增 sequoiadb_execution_mode 配置参数
  - 新增 information_schema_tables_stats_cache_first 配置参数
  - sdb_sql_ctl 工具端口参数 -p 调整为 -P

**主要特性：**

NA

**性能优化：**

- 存储引擎
  - 优化过滤的记录数超过 CL 总记录数的 10% 时，执行 count 操作的性能

- SQL 引擎
  - 优化从 information_schema.tables 查询统计信息的性能

**工具优化：**

- SQL 引擎
  - 修复某些 auto.cnf 配置下，sdb_sql_ctl 工具添加实例到实例组会报错的问题
  - 实例组在开启 sequoiadb_execute_only_in_mysql 的情况下，SQL 实例的元数据能够同步

**解决重要Bug：**

- 存储引擎
  - 修复指定节点角色为所有节点时，查询回收站快照可能造成节点 crash 的问题
  - 修复执行 split 操作，可能造成节点 crash 的问题
  - 修复 sdbcm 进程创建的子进程可能变成僵尸进程的问题
  - 修复同步日志满，备节点归档日志归档失败的问题
  - 修复回收站项目已满，删除 CS/CL 可能导致报错 -147 的问题

- SQL 引擎
  - 修复 SequoiaDB 回收站已满时不正确的错误处理
  - 修复对同一张表有跨实例的 DDL 与 DQL 并发操作可能导致 crash 的问题
  - 修复异常场景下可能有日志未被实例组回放线程同步的问题
  - 修复存储过程中创建用户使用明文密码，密码内容不正确的问题
  - 修复无事务模式下，执行 INSERT INTO ... SELECT ... 语句可能触发的空指针异常
  - 修复在特定复杂 WHERE 条件下，查询使用 GROUP MIN MAX 访问计划有错误结果的问题
  - 修复有聚集函数时，direct_limit 优化可能错误施加，导致错误结果的问题
  - 修复查询慢查询日志表走 direct_count 优化查询时导致 crash 的问题
  - 修复复合索引首字段匹配条件为 IS NULL 时可能导致 crash 的问题
  - 修复 MySQL 分区表指定分区名并发查询可能结果集缺失的问题
  - 修复若干潜在的变量未初始化、内存非法读取问题

[^_^]:
    本文使用的所有引用及链接
[quickstart]:manual/Quick_Start/quick_deployment.md
