SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，产品引擎采用原生分布式架构，100% 兼容 MySQL，支持完整的 ACID 和分布式事务。同时 SequoiaDB 还提供多模（multi-model）数据库存储引擎，原生支持多数据中心容灾机制，是新一代分布式数据库的首选。

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据增删改查的基本语法、数据库运维管理的基本策略，以及性能调优和问题诊断的相关思路。

[快速使用 SequoiaDB][quickstart]

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
