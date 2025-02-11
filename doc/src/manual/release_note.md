SequoiaDB 巨杉数据库是一款金融级分布式数据库，产品引擎采用原生分布式架构，协议级兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用 SequoiaDB][quickstart]

##注意事项##
- 从 3.4.4/3.6/5.0.3 及早期版本滚动升级到 3.4.5/3.6.1/5.0.4 及之后的版本时，从 SQL 引擎执行的 INSERT 操作会存在失败。因此滚动升级的过程中需保证优先完成存储引擎的升级，然后再进行 MySQL/MariaDB 实例的升级。
- 从 3.4.4/3.6/5.0.3 及早期版本升级到 3.4.5/3.6.1/5.0.4 及之后的版本，如果集群会扩展为 X86 和 ARM 架构混合部署，则在升级前版本上创建的、使用 double 类型字段作为 hash 分区键的集合，需要进行重建，否则可能会出现数据无法正确访问的问题。可通过查询 SDB_SNAP_CATALOG 快照，根据集合使用的 hash 算法版本号（InternalV 字段）判断，对于该版本号小于 4 的集合需要进行处理。

##SequoiaDB version 3.4.13 版本说明##

**接口变更：**

- 存储引擎
  - 重选举支持设置选举级别，阻塞不同的写操作：
     - Level 1: 等待当前写操作结束，并阻塞后续写操作
     - Level 2: 等待写游标(大对象操作)结束
     - Level 3: 等待事务结束
  - sdbcm 新增 ValidTimeThreshold 参数，设置异常节点自动重启间隔时间。如果在间隔时间内，sdbcm 自动重启异常节点次数超过 RestartCount 参数的限制，则 sdbcm 不会再重启该异常节点；
  - 快速部署工具支持 MariaDB 快速部署；
  - 集合 ReplSize 默认值改为 2；
  - SDB 支持复制日志文件的缓存清理；
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

- SQL 引擎
  - MySQL performance_schema 支持设置皮秒时间精度为纳秒；

**主要特性：**

- 存储引擎
  - 重选举不阻塞读操作；
  - 残留和空 CS 自动清理优化；

- SQL 引擎
  - MySQL 升级至 5.7.44；

**性能优化：**

- 存储引擎
  - 优化节点启动/停止流程，加速节点启动/停止；
  - 加载集合空间内存优化；
  - 当集合空间只有一个集合时，dropCL 优化为 dropCS；
  - 优化索引统计信息子表采样率；

- SQL 引擎

  NA

**工具优化：**

- 存储引擎
  - sdbdmsdump 工具优化改进；

- SQL 引擎

  NA

**解决重要Bug：**

- 存储引擎
  - 修复开启容错，备节点异常时，Lob 写操作无法降级处理的问题；
  - 修复创建 CS 文件/drop/truncate 产生 DEADSYNC 和 SLOWNODE 的问题；
  - 修复 ARM 平台上数值转换由于编译器优化导致结果不正确的问题；
  - 修复索引平衡导致节点 coredump 的问题；
  - 修复编目节点发送消息报-85错误导致所有协调节点连接卡住的问题；
  - 修复集合空间 DataCommitted 标记不正确，导致发生非预期的全量同步的问题；
  - 修复大并发执行快照采集导致协调节点主线程消息队列阻塞的问题；
  - 修复开启慢查询执行 query 和 killcontext 协调节点 coredump 的问题；
  - 修复执行内置 SQL 带命令位置参数报错的问题；
  - 修复 transactionon 为 false 时，执行事务相关快照导致节点 coredump 问题；
  - 修复访问计划索引评估出错的问题；
  - 修复等值匹配时访问计划索引选择率不准确的问题；
  - 修复 C++ 驱动 cl.query() 与 cl.interrupt() 并发执行偶现卡死的问题；

- SQL 引擎
  - 修复索引读不能正确处理夏令时的问题；
  - 修复索引统计信息未被正确清除的问题；
  - 修复 flush tables 导致实例 coredump 的问题；

##SequoiaDB version 3.4.12 版本说明##

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
  - 修复数据切分过程中，备节点 LSN 差距越来越大，导致全量同步的问题

- SQL 引擎
  - 修复 MySQL BKA join 缺少结果集的问题

##SequoiaDB version 3.4.11 版本说明##

**接口变更：**

- 存储引擎
  - sdb shell 新增 memtrim() 接口

**主要特性：**

- 存储引擎
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
  - 修复 SQL 隔离级别为 SERIALIZABLE 时导致 crash 的问题

##SequoiaDB version 3.4.10 版本说明##

**接口变更：**

NA

**主要特性：**

- SQL 引擎
  - 支持 MySQL 5.7.42
  - 支持 MariaDB 10.4.28

**性能优化：**

- 存储引擎
  - 优化过滤的记录数超过 CL 总记录数的 10% 时，执行 count 操作的性能

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复同步日志满，备节点归档日志归档失败的问题
  - 修复 sdbcm 退出后，使用后台模式执行任意 sdb shell 命令可能卡住的问题
  - 修复多次节点创建不成功，再次创建节点可能因 nodeID 满而失败的问题
  - 修复使用 java 驱动调用 closeAllCursors() 接口后，游标继续执行 getNext() 不报错的问题

- SQL 引擎
  - 修复存储过程中使用明文密码可能有错误结果的问题
  - 修复跨实例间 DDL 与查询并发可能 crash 的问题
  - 修复 direct_update(direct_delete) 执行 index merge 访问计划时可能有错误结果的问题
  - 修复 MariaDB 查询 information_schema.system_variables 视图会 crash 的问题

##SequoiaDB version 3.4.9 版本说明##

**接口变更：**

- 存储引擎
  - 使用 REST 驱动删除集合空间时，支持添加 option 参数
  - insert 接口新增 flag，用于控制发生唯一键冲突时的行为
  - PHP 驱动 getIndexStat 接口支持 Detail 参数

**主要特性：**

- SQL 引擎
  - 支持 MySQL 5.7.40
  - 支持 Catalog Simulation 工具以帮助定位诊断执行计划不准确的问题

**性能优化：**

- 存储引擎
  - 优化集合中同时存储普通记录与 LOB 数据时的写入性能

**工具优化：**

- SQL 引擎
  - 实例组新建实例时，优化全量同步 DUMP 表数据时的加锁行为

**解决重要Bug：**

- 存储引擎
  - 修复数据压缩导致消耗 CPU、IO 过高的问题
  - 修复 $rename 操作将字段名改为记录中已存在字段时，可能导致主备节点不一致的问题
  - 修复挂载子表的名称长度不能超过 127 字节的问题

- SQL 引擎
  - 修复 MySQL 查询时使用了内部临时表可能导致分页查询结果不正确的问题
  - 修复 INDEX MERGE 查询时带 limit 可能导致查询结果不正确的问题
  - 修复实例组在 SQL 表与 SequoiaDB 集合版本不一致下，PREPARE/EXECUTE 等语句报 -357 错误的问题
  - 修复管理工具脚本无法识别带空格的用户密码问题
  - 修复相关子查询时，父查询块表条件压给子查询块表场景下，可能查询结果不正确的问题
  - 修复 GROUP BY 使用 MRR 做多范围 loose scan 时结果集缺失的问题
  - 修复 OPTIMIZER_SWITCH 中 MRR 为 OFF 时，逆序查询可能执行报错的问题
  - 修复 MariaDB 在 GROUP MIN/MAX 执行计划时可能 crash 的问题

##SequoiaDB version 3.4.8 版本说明##

**接口变更：**

- 存储引擎
  - sdb shell 及 C++ 驱动的创建用户的接口支持给用户指定角色
  - 驱动的 getIndexStat 接口支持使用 detail 参数，用于返回索引频繁集
  - java 驱动新增使用密码文件创建连接的接口
  - java 支持 SCRAM-SHA256 鉴权机制
  - java 驱动的 createLobID 接口中日期格式化串调整

- SQL 引擎
  - 新增 sequoiadb_stats_cache_level 配置参数
  - 新增 sequoiadb_stats_flush_time_threshold 配置参数

**主要特性：**

- 存储引擎
  - 支持在创建用户时指定内置的管理员或监控角色，实现账户的权限控制
  - 支持节点编目信息缓存自动清理，控制内存使用量
  - 提供 LOB 相关的容量、负载监控指标项，详见集合空间、集合、会话等快照
  - 在数据库及会话快照中增加事务提交及回滚次数，以及数据库操作、慢操作的次数统计
  - 支持查看索引频繁集（MCV）
  - 集合空间及集合元数据中增加创建及修改时间

- SQL 引擎
  - 索引统计信息支持对接 SequoiaDB 索引频繁集以提升索引统计信息精确度
  - 统计信息支持过期自动失效

**性能优化：**

- SQL 引擎
  - LIKE 条件支持部分字符串函数下压以提升 LIKE 查询性能

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复部分操作中断的情况下可能造成 context 泄漏的问题
  - 修复容灾工具 merge 脚本可能报错的问题
  - 修复用户级审计配置失效的问题
  - 修复内存耗尽等极端场景下的部分节点异常的问题

- SQL 引擎
  - 修复会话并发 kill <connection_id> 可能 coredump 问题.
  - 修复分区表指定某分区做 join 查询失败问题
  - 修复实例组元数据同步若干已知问题
  - 修复 sysdate 函数做 count 查询结果集不正确问题.


##SequoiaDB version 3.4.7 版本说明##

**接口变更：**

NA

**主要特性：**

- 存储引擎
  - Flink 连接器新增支持 UPDATE/DELETE 类型操作
  - Flink 连接器为入数操作提供贴源数据时序一致性保证能力

- SQL 引擎
  - 支持 MySQL 5.7.39

**性能优化：**

NA

**工具优化：**

NA

**解决重要Bug：**

- 存储引擎
  - 修复操作被中断场景可能导致上下文泄漏，内存上涨的问题
  - OpenSSL 库版本升级到 1.1.1o，解决已知漏洞

- SQL 引擎
  - curl 库版本升级到 7.83.1，解决已知漏洞
  - OpenSSL 库版本升级到 1.1.1o，解决已知漏洞
  - 修复 LIST 分区表的某个分区与 KEY 分区表做 JOIN 时，误报不支持指定分区查询的问题


##SequoiaDB version 3.4.6 版本说明##

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
  - 修复部分场景下 LOB 文件大小异常的问题
  - 修复部分场景下使用容灾脚本合并集群报错的问题

- SQL 引擎
  - 修复批量插入失效问题
  - 修复部分场景下多范围索引查询可能 coredump 问题

- SAC
  - 移除 SAC 中 SQL 插件对 Logback 的依赖，消除漏洞影响

##SequoiaDB version 3.4.5 版本说明##

**接口变更：**

- 存储引擎
  - 插入操作返回的结果中增加 ModifiedNum 字段

**主要特性：**

- 存储引擎
  - 支持在 X86、ARM 架构的主机上进行灵活的混合部署
  - 诊断日志对用户数据加密，可使用解密工具 sdbsecuretool 进行解密查看
  - 支持插入冲突时直接根据指定的规则对记录进行更新（当前仅对接 SQL 引擎）
  - 新增 Flink 连接器，提供 SequoiaDB 与流式框架对接的能力
  - 增强节点运行时对于读写磁盘错误的检测能力，在故障情况下及时触发报错或切换
  - 优化集合空闲空间管理算法，减少集合空间膨胀

**性能优化：**

- SQL引擎
  - INSERT ... ON DUPLICATE KEY UPDATE 直接使用存储引擎支持的对应功能，优化性能
  - 优化 IN 条件部分场景下 UPDATE/DELETE/COUNT 的性能
  - 优化部分场景下 SELECT COUNT 的性能

**工具优化：**

- SQL 引擎
  - sdb_sql_ctl 工具新增实例时，实例名风格规范为字母开头，并且可包含字母、数字和下划线

**解决重要Bug：**

- SQL 引擎
  - 完善版本升级或回退时的兼容性处理
  - 修复 INSERT INTO ... ON DUPLICATE 并发时可能出现错误的问题
  - 修复大数据场景下，根据索引字段执行 ORDER BY ... LIMIT ... 查询时，无法使用索引有序性的问题
  - 修复分区表存在的若干已知问题
  - 修复 ALTER 添加带默认值的列时可能重置底层数据问题
  - 修复 IS/NOT NULL 条件不能下压 COUNT 问题
  - 修复 ha_inst_group_chpass 工具在变更密码后实例可能无法重启问题
- 存储引擎
  - 修复 java 驱动中 Date 类型数据时间部分被截断的问题
  - 修复 java 中 Date 类型存在的时区问题
  - 修复集合重命名被其它集合上的操作阻塞的问题
  - 修复 TRUNCATE 操作与并行按 Segment 读取集合数据时可能报错的问题
  - 修复节点因 SIGHUP 信号退出的问题
  - 修复主机内存耗尽情况下的若干稳定性问题
  - 修复个别子表缺少索引导致其它有索引的子表走表扫描的问题
- SAC
  - 升级 SQL 插件的 jackson-databind，修复漏洞

##SequoiaDB version 3.4.4 版本说明##

**接口变更：**

- SQL引擎
  - 支持 MySQL 5.7.34
- C++ 连接池支持使用密码文件，支持修改协调节点的地址、用户名及密码
- 集合空间新增 getDomainName() 及 listCollections() 接口
- 备份支持参数 MaxDataFileSize，用于指定备份文件大小

**主要特性：**

- SQL 引擎
  - 新增 MySQL 实例与 SDB 元数据映射功能
  - 新增 MySQL 实例与 SDB 数据组强绑定功能
- 提供死锁检测快照，方便发现事务死锁并进行处理

**性能优化：**

- SQL引擎
  - 优化 MySQL NLJOIN 性能，减少下压次数，提升查询性能
  - 优化 MySQL 与 SDB 连接策略，优先连接本地 coord 节点
  - 优化 MySQL 优化下压策略，减少不必要的下压条件以提升查询性能
  - 优化 MySQL 某些场景下的条件下压次数，提升查询性能
- 优化 count 操作的网络交互，提高 count 操作性能
- 优化事务中大批量删除记录时的内存使用

**工具优化：**

- 优化导入脚本对特殊字符的处理
- 优化导出脚本对分割符的处理

**解决重要Bug：**

- 修复 MySQL 部分类型在跨类型比较时结果非预期的问题
- 修复 MySQL 部分场景下 count 查询没有下压的问题
- 提升 MySQL 实例组功能稳定性
- openssl 库从版本升级到 1.1.1k，解决已知漏洞
- 修复集合非常多的情况下获取集合快照占用内存过多的问题

##SequoiaDB version 3.4.3 版本说明##

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

##SequoiaDB version 3.4.2 版本说明##

**接口变更：**

- SQL引擎
  - 支持原生分区语法 PARTITION BY
  - 分区表支持 CHECK / REPAIR 操作
  - 支持 geometry 数据类型
  - sequoiadb_use_transaction 改为 session 级别
- $elemMatchOne 和 $elemMatch 支持匹配数组非嵌套元素
- 数据库的服务端口支持开关配置

**主要特性：**

- SQL引擎
  - 新增 SQL 实例组功能
     - 实例组内所有实例之间自动同步元数据，实现元数据一致性
     - 实例组内所有实例同时对外提供读写服务
     - 实例组内所有实例对外提供高可用 HA 服务
     - 实例组同时支持 MySQL 实例和 MariaDB 实例
  - MariaDB 支持分布式 sequence
  - MariaDB 支持闪回查询功能
- 索引支持"Not Array"属性，开启"Not Array"属性时该索引对应的所有列不允许为数组
- SAC增加操作日志管理功能，可以查看详细操作信息
- 新增获取索引统计信息的功能

**性能优化：**

- SQL引擎
  - 支持条件部分下压
- 支持"Index Cover"能力：查询操作所有字段都命中索引，可以直接使用索引键值，而不需要读取记录，提升性能
- 在高并发访问时，降低编目节点访问压力，提升整体性能
- 在数据查询操作中减少一次"GetMore"消息交互，提升查询性能
- 事务下索引查询 count() 优化
- 并发回放事务的 commit 和 rollback 日志
- 优化内存池及内存监控
- 优化查询命中单个子表的性能

**工具优化：**

- SAC
  - 集合页面新增表扫描次数、索引扫描次数显示
  - SequoiaPerf 增加告警管理功能，可以自定义告警级别，触发条件等配置
- 优化导入工具导入多层嵌套 json 性能过慢的问题
- sdbimprt、sdbexprt 支持空的字符分隔符
- 导入工具增加新参数，指定索引冲突处理
- sdbdpsdump 增加过滤事务id的能力
- 新增数据重分布服务，能实现按周期、指定时间进行数据自动重分布
- 提供审计日志服务，实现审计日志集中管理、查看
- 批量导入导出工具，可对实现对多个集合空间或集合的数据进行导入和导出
- HA工具去除对用户名密码依赖

**解决重要Bug：**

- 修复 数据节点升主时回滚事务可能与新发起自动提交事务卡住的问题
- 修复 PostgreSQL 引擎查询索引失败后，释放BSON内存时发生 coredump 的问题
- 修复 TRACE 跟踪过程中无法停止节点的问题
- 修复 特定场景下参数"tranreplsize"配置不生效的问题
- 修复 createAutoIncrement 创建自增字段过程中可能的内存泄漏问题

##SequoiaDB version 3.4.1 版本说明##

**接口变更：**

- SQL引擎
  - 兼容 MariaDB 协议；
  - 增加参数 sequoiadb_rollback_on_timeout ，开启时当事务锁超时回滚整个事务；
  - sdb_sql_ctl 改名为 sdb_mysql_ctl 和 sdb_pg_ctl；
- 全文索引支持Elasticearch 6.8.5版本；
- 系统 limit 配置支持 stack size，并统一单位为 byte；

**主要特性：**

- SQL引擎
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
  - 支持 LIMIT 算子下推；

**工具优化：**

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
- 修复REST接口内存泄漏问题；
- 修复Java驱动使用中文密码鉴权失败的问题；
- 修复当集合数量超过6万个时执行集合快照失败的问题；
- 修复在备节点进行事务 COUNT 报-104的错误；
- 修复 TRUNCATE 和 DROP INDEX 并发回放时导致节点异常的问题；
- 修复导出工具开启 ```--withid false``` 导入 JSON 格式不生效的问题；


**注意事项：**

- 对于使用了全文检索的环境，升级过程中要同时升级并重启适配器进程。由于 3.0 之后的版本对全文索引相关机制进行了调整优化，如果升级前版本为 3.0，在升级过程中，需要在停数据节点前，先将所有适配器停止，然后再进行节点升级，或者在升级前将全文索引删除，在升级完成后重建。

##SequoiaDB version 3.4 版本说明##

**接口变更：**

- MySQL引擎
  - 配置项 *sequoiadb_use_partition* 更名为 *sequoiadb_auto_partition* ；
  - 废弃配置项 *sequoiadb_optimizer_select_count* ；
  - 新增配置项 *sequoiadb_use_transaction*、 *sequoiadb_optimizer_options*；
- REST接口支持返回标准JSON格式；
- 提供标准S3兼容的对象访问接口，实现“桶”和“对象”操作以及“多版本控制”；
- 提供 SQL 化监控视图对数据库进行监控，可以使用SQL的各种能力灵活筛选和组合监控数据，提升监控的易用性；
- 提供 MariaDB 兼容的 SQL 接口；

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
- 大对象存储支持按时间序进行表分区，提升对大对象的存取和管理能力，可以快速按时间进行归档和清理；
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



[^_^]:
    本文使用的所有引用及链接
[quickstart]:manual/Quick_Start/quick_deployment.md
