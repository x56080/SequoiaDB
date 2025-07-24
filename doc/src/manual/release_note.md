<<<<<<< HEAD
SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，产品引擎采用原生分布式架构，100% 兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。
=======
SequoiaDB 巨杉数据库是一款金融级分布式数据库，产品引擎采用原生分布式架构，协议级兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用 SequoiaDB][quickstart]

##注意事项##
- 从 3.4.4/3.6/5.0.3 及早期版本滚动升级到 3.4.5/3.6.1/5.0.4 及之后的版本时，从 SQL 引擎执行的 INSERT 操作会存在失败。因此滚动升级的过程中需保证优先完成存储引擎的升级，然后再进行 MySQL/MariaDB 实例的升级。
- 从 3.4.4/3.6/5.0.3 及早期版本升级到 3.4.5/3.6.1/5.0.4 及之后的版本，如果集群会扩展为 X86 和 ARM 架构混合部署，则在升级前版本上创建的、使用 double 类型字段作为 hash 分区键的集合，需要进行重建，否则可能会出现数据无法正确访问的问题。可通过查询 SDB_SNAP_CATALOG 快照，根据集合使用的 hash 算法版本号（InternalV 字段）判断，对于该版本号小于 4 的集合需要进行处理。
<<<<<<< HEAD

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
=======

##SequoiaDB version 5.0.3 版本说明##

**接口变更：**

- 存储引擎
  - 事务查询支持 READ FOR SHARE 模式，对已读记录持共享锁
  - 支持读写分离模式下，严格指定访问节点的角色
  - 驱动支持 INSERT/DELETE/UPDATE 返回操作记录数
  - 驱动支持查询集合空间所属域及包含的集合列表

**主要特性：**

- SQL 引擎
  - 新增建表语句选项，支持映射到存储引擎中已存在的集合
- 存储引擎
  - 新增上下文清理机制，防止上下文泄露影响系统稳定性
  - 新增事务锁升级机制，降低大事务内存使用量
  - 索引元数据纳入编目节点统一管理，增强索引一致性
  - 提供独立索引能力，支持根据业务需要在特定的节点上创建索引
  - 新增回收站能力，支持 DROP CS/DROP CL/TRUNCATE CL 操作时数据的回收和快速恢复
  - 新增 Flink 连接器，提供 SequoiaDB 与流式框架对接的能力
  - 增强节点运行时对于读写磁盘错误的检测能力，在故障情况下及时触发报错或切换

**性能优化：**

- SQL 引擎
  - SELECT ... LOCK IN SHARE MODE 更新为使用共享锁，提升操作并发性能
  - 优化部分场景下 SELECT COUNT 的性能
  - 优化多字段 IN 查询以及多字段多范围查询时的性能
  - 优化联合索引关联查询时，支持 BKA JOIN 算法以提升对应 JOIN 操作的性能
  - 优化提升 OR 条件下 INDEX MERGE 的性能

**工具优化：**

- SQL 引擎
  - 新增实例组用户密码变更工具 ha_inst_group_chpass，支持变更实例组用户密码
  - 新增元数据映射初始化工具 sql_enable_mapping，支持创建表以映射到存储引擎中已存在的集合
  - 新增元数据映射查看工具 sql_get_mapping，支持查询 SQL 实例表与存储引擎集合之间的映射关系
  - sdb_sql_ctl 工具新增实例时，实例名风格规范为字母开头，并且可包含字母、数字和下划线
  - sdb_sql_ctl 工具 listinst 命令增加 status 参数，用于查看实例启动时间
  - ha_inst_group_list 脚本新增 --data-group 字段，用于显示与实例组绑定的存储引擎中的分区组
- 存储引擎
  - sdbimprt 工具返回唯一索引冲突数

**解决重要Bug：**

- SQL 引擎
  - 完善版本升级或回退时的兼容性处理
  - 修复 INSERT INTO ... ON DUPLICATE 并发时可能出现错误的问题
  - 修复实例组场景下，实例关闭事务时，DDL 语句依然会产生事务操作的问题
  - 修复大数据场景下，根据索引字段执行 ORDER BY ... LIMIT ... 查询时，无法使用索引有序性的问题
  - 修复分区表存在的若干已知问题
- 存储引擎
  - 修复集合重命名被其它集合上的操作阻塞的问题
  - 修复 TRUNCATE 操作与根据集合元数据按块读取数据时可能报错的问题
  - 修复节点因 SIGHUP 信号退出的问题
  - 修复主机内存耗尽情况下的若干稳定性问题
- SAC
  - 升级 SQL 插件的 jackson-databind，修复漏洞

##SequoiaDB version 5.0.2 版本说明##

**接口变更：**

- SQL引擎
  - MySQL/MariaDB 增加 preferredinstance 配置参数
- fap 支持 findAndModify 功能
- fap 支持 bulkWrite 功能

**主要特性：**

- SequoiaDB 增加数据源功能

**性能优化：**

- SQL引擎
  - 优化 MySQL 索引查询性能
  - 优化 MySQL multistatement 数据插入性能
- 优化并发回放性能
- 优化主子表下对切分键排序查询的性能

**工具优化：**

- MySQL 默认配置 lower_case_table_names 为 1（表名存储在磁盘是小写，比较时不区分大小写 ）
- MySQL 实例和 PostgreSQL 实例默认日志路径从安装目录调整到数据目录下

**解决重要Bug：**

- 修复 MySQL 部分场景下条件下压不正确的问题
- 修复 MySQL 实例组功能在多实例并发极限场景下，多个实例之间元数据不同步的问题
- 修复 PostgresSQL 在特殊查询条件下造成内存泄漏的问题
- 修复 引擎在内存严重不足时导致程序退出的问题

##SequoiaDB version 5.0.1 版本说明##

**接口变更：**

- SQL引擎
  - 配置项 sequoiadb_optimizer_options 增加 direct_sort/direct_limit 项，以控制 order by/limit 是否下压；
  - 兼容 MariaDB 协议；
  - 增加参数 sequoiadb_rollback_on_timeout ，开启时当事务锁超时回滚整个事务；
  - sdb_sql_ctl 改名为 sdb_mysql_ctl 和 sdb_pg_ctl；
- 全文索引支持Elasticearch 6.8.5版本；
- 系统 limit 配置支持 stack size，并统一单位为 byte；

**主要特性：**

- SQL引擎
  - 新版实例元数据同步机制，旧版元数据同步使用 meta_sync 脚本工具实现，有诸多限制。新版实例元数据同步使用 mysql 插件引擎实现，简化安装部署，解决旧版同步工具限制问题；
  - 支持 MySQL 5.7.31；
  - MySQL 5.7.28/5.7.31/MariaDB 10.4.6 的 OpenSSL 升级到 1.1.1g ；
  - 支持 geometry 空间数据类型；
  - 支持 PARTITION BY 语法；
  - 支持配置安全密码；
- 分区组内数据节点心跳支持 UDP/TCP 两种协议，并能实现自动探测和切换；
- 提供基于SCRAM-SHA-256认证机制的安全鉴权协议，可防止重放攻击、网络窃听攻击、数据破解、服务端伪造等；
- 提供分区组容错熔断机制，通过配置开启错误和风险智能检测，并提供“熔断”、“半容错”和“全容错”三种容错级别，实现高可用；
- 引入读写分离过期机制，实现“读写分离”和“读我所写”的自动切换，既满足数据的一致性，又实现负载分离；
- 增加 update one / delete one 功能；
- 更新符支持用一个字段更新另一个字段；
- 支持 LOB 并发读写；

**性能优化：**

- 优化事务锁的性能和事务老版本清理的性能；
- 优化节点启动性能；
- 优化索引匹配，优先选择 $et 匹配操作字段对应的索引；
- 优化分区命中算法，提升分区路由性能；
- SQL引擎
  - 支持部分条件下压，优化部分条件不满足下压时的性能表现；
  - direct_count 模式支持带条件语句，优化带条件 select count 语句的性能表现；
  - 支持 order by/limit 下压，优化 order by/limit 语句的性能表现；

**工具优化：**

- STP 查询工具 stpq，支持查询 STP 节点状态、同步信息等；
- SDB SHELL支持安全密码、交互密码和密码无痕迹功能；
- 导入工具支持空字符串的 Decimal 类型；
- 导入工具支持将 Decimal 转换为其它类型；
- 导入工具支持将 null 转换为 Date/Timestamp 类型；
- 支持 TRACE 的结果导出到客户端本地；
- SAC
  - 提供图形化性能监控工具(SequoiaPerf)，简化端到端的慢查询性能分析；

**解决重要Bug：**

- 修复SQL引擎实例数据同步时，对 ```create table A select * from B``` 语句数据量翻倍的问题；
- 修复SQL引擎采用 COPY 算法 ALTER TABLE 主子表时丢失子表的问题；
- 修复SQL引擎查询大量 TEXT 类型记录时内存消耗过大的问题；
- 修复从旧版本 SequoiaDB 升级到 5.0 SequoiaDB 时的问题；
- 修复REST接口内存泄漏问题；
- 修复Java驱动使用中文密码鉴权失败的问题；
- 修复当集合数量超过6万个时执行集合快照失败的问题；
- 修复在备节点进行事务 COUNT 报-104的错误；
- 修复 TRUNCATE 和 DROP INDEX 并发回放时导致节点异常的问题；
- 修复导出工具开启 ```--withid false``` 导入 JSON 格式不生效的问题；

##SequoiaDB version 5.0 版本说明##
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

**接口变更：**

- SQL 引擎
  - sdb_sql_ctl 工具新增 load-stats 参数，用于启动时加载统计信息

**主要特性：**

<<<<<<< HEAD
- 存储引擎
  - 提供数据恢复工具 sdbrevert

- SQL 引擎
  - MySQL 支持 v5.7.42
  - SequoiaDB 存储引擎的表支持查看建表时间
  - 实例组集合空间支持指定数据域
=======
- MySQL引擎
  - 支持无事务模式，能够实现批量非事务的高性能操作；
  - 完善表、列、主键和索引的修改操作；
  - 支持多实例元数据实时同步，提供高可用能力；
  - Update/Delete/Count/Autocommit下推优化，提升性能；
  - 支持 Insert ... on duplicate key update ... 语法；
- 支持基于多版本的事务并发控制 ( MVCC, Multi Version Concurrency Contral )
- 支持全局一致性事务
- 支持全局逻辑时间 ( STP 逻辑时间协议 )
- Insert/Update/Delete等支持返回记录数和详细错误信息；
- 插入数据支持重复键替代( insert ... on duplicate replace )；
- 访问计划实现自动过期清理，以及对 $in 操作进行参数化和缓存；
- 全文索引支持字符串数组，以及 $or 和 $not 操作；
- 索引支持 not null 约束；
- 命令位置参数支持 InstanceID ；
- 大对象存储支持按时间序进行表分区，提升对大对象的存取和管理能力，可以快速按时间进行归档和清理；
- 大对象List操作支持过滤条件和精准匹配；
- 重选举支持指定节点；
- 复制日志支持开启全量模式和时间字段，可以通过工具进行增量数据抽取；
- 多唯一索引的集合支持副本节点并发数据同步和重放；
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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

<<<<<<< HEAD
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
=======
**注意事项：**

- 对于使用了全文检索的环境，升级过程中要同时升级并重启适配器进程。由于 3.0 之后的版本对全文索引相关机制进行了调整优化，如果升级前版本为 3.0，在升级过程中，需要在停数据节点前，先将所有适配器停止，然后再进行节点升级，或者在升级前将全文索引删除，在升级完成后重建。

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

[^_^]:
    本文使用的所有引用及链接
[quickstart]:manual/Quick_Start/quick_deployment.md
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
