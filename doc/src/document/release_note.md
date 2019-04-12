SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，产品引擎采用原生分布式架构，100%兼容 MySQL 语法和协议，支持完整 ACID 和分布式事务。同时，SequoiaDB 还提供多模（multi-model）数据库存储引擎，数据库引擎原生支持多数据中心容灾机制。  
SequoiaDB 是新一代分布式数据库的首选。

[快速使用SequoiaDB](quickstart.md)

##SequoiaDB version 2.8.7 版本说明##

**接口变更：**

- db.createUsr() 创建用户接口支持设置用户 AuditMask 属性

**功能优化：**

- 提供基于用户级的审计日志能力


##SequoiaDB version 2.8.6 版本说明##

**接口变更：**

- 新增 PostgreSQL 连接器对增强的 sessionattr 设置选项的支持
- 新增 sdbimprt 工具对 timestamp 类型字段单独指定精度的能力
- 新增 sdbimprt 工具对时区的支持
- 新增 Java 驱动中 ReplicaGroup 和 Node 判断组或节点是否存在的接口
- 扩展单节点集合空间的限制，由 4096 提升到 16384

**工具优化：**

- 优化导入工具的 unicode 编码性能

**性能优化：**

- 优化进程正常重启后事务日志恢复性能


**解决重要bug：**

- insert 和 truncate 在某些情况下可能发生锁等待
- 并发创建索引时在某些极端情况下可能出错
- or 条件中同时包含分区键和非分区键字段查询返回结果错误
- 分区表使用特定条件查询，匹配数据不完整
- 使用 $exists 选择集合中嵌套数组中的元素，匹配记录不正确
- 使用内置SQL插入timestamp类型记录，1970年前的时间戳记录异常
- 选择符elemMatch只对数组生效，对嵌套obj不生效
- select 查询时做算术运算并排序，返回结果不正确

**其它优化：**

- 优化排序空间开销
- 优化自动切分算法使数据分配更均匀
- 命令位置参数 Role 在开启节点过滤时生效

##SequoiaDB version 2.8.5 版本说明##

**接口变更：**

- Java驱动连接池提供会话属性设置相关接口
- SDB SHELL和所有驱动提供获取会话属性接口
- 参数cachemergesz、sparsefile、overflowratio和extendthreshold支持实时生效
- 参数maxcachesize、maxcachejob、maxsyncjob等参数提供动态生效能力

**特性开发：**

- 外部会话提供超时以及访问隔离的能力

**工具优化：**

- 导入工具支持NaN的数值
- 灾备工具可用性优化，减少重选举操作
- sdbexprt工具增加replace参数，默认不覆盖数据文件

**性能优化：**

- 优化缓存页回收和刷盘调度机制
- 提供写缓存带超时阻塞的能力

**解决重要bug：**

- 集合切分在元数据变更阶段后源节点故障重启导致该切分无法完成
- Java驱动设置socket超时在发生超时后导致消息收错
- 并发多个集合执行LOB读写操作，其中一个集合执行listLob失败报-268错误
- 并发读写删LOB操作，在备节点回放LOB日志失败报-268错误
- 在开启缓存在DirectIO模式下进行大并发LOB写操作，在删除集合空间时卡住
- 集合切分LOB过程中truncate集合导致该切分一直不能结束
- 使用$cast转换{$decimal:"MAX"}为int64使数据节点core
- 数据切分同时删除数据，切分过程中源组删除了某记录，而目标组未同步删除该记录

**其它优化：**

- 修正System.getDiskInfo在Ubuntu17上执行报-10的错误
- ossGetDiskInfo存在句柄泄漏
- 数据节点上snapshot session显示的name超64字节被截断
- 插入NAN的特殊decimal值，转换为double类型后使用toObj()输出失败
- 超2000会话压力下，在会话退出时出现极小窗口的非法访问导致节点崩溃
- 内存SQL中select语句指定a字段和a.b字段，a字段不生效
- 节点参数auditmask的默认值不正确
- 修正协调节点上SYSTEM快照结果值不准确的问题，没有按节点去重

##SequoiaDB version 2.8.4 版本说明##

**接口变更：**

- 内置sql支持oid/timestamp/is not
- 统一Python驱动异常处理
- Python驱动client.create_replica_cata_group()接口参数改为可选
- Python驱动增加复制组相关的接口
- Python驱动增加域相关的接口

**工具优化：**

- sdbexprt在指定分隔符时与sdbimprt处理方式保持一致
- 一致性校验工具校验错误数据时挂掉
- sdbimprt使用空格或制表符作为字段分隔符时导入的记录可能有错误
- dr_ha脚本重启节点操作并行化
- dr_ha脚本支持剔除复制组故障节点
- sdbexprt分隔符处理错误

**性能优化：**

- 访问计划缓存优化
- 将LOBM文件的PageSize大小从256字节调整为64字节
- 优化C#驱动批插性能
- 索引性能优化

**解决重要bug：**

- 在磁盘空间满时备节点出现日志损坏并导致全量同步
- 备节点磁盘满时创建cs/cl与读写LOB并发在回滚删除LOB日志失败时该备节点宕机
- 插入相同数据并多次停启主节点导致主备数据不一致
- 开启数据压缩时创建字典的并发操作存在问题
- 集群整体掉电导致数据节点rebuild失败无法启动
- 并发增删改操作过程中集群掉电故障恢复后数据组主节点rebuid过程中频繁重启

**其它优化：**

- preferinstance默认值从A改成M
- Collection.listLobs()卡死
- 反复停启数据主节点导致主节点dms元数据totalRecords与实际数据个数不一致
- 日志归档开启压缩时节点宕机
- 并发执行事务过程中执行事务快照导致节点宕机
- 插入大量LOB数据时删除集合空间导致节点崩溃
- 查询连接较多时数据节点卡住
- 节点故障重启后rebuild过程中死锁
- forceStepUp()可能会导致节点宕机
- 多个集合并发执行LOB随机读写时数据主节点宕机

##SequoiaDB version 2.8 版本说明##

**接口变更：**

- 匹配符中$size和$type变更为函数，即 {a:{$type:1}}=>{a:{$type:1,$et:1}}；{a:{$size:1}}=>{a:{$size:1,$et:1}}
- C#驱动提供LOB的Read/Write带指定偏移和长度的接口
- Java驱动提供LOB的流式输入输出接口
- Python驱动支持Python 3.5，同时放弃对Python 2.6的支持
- C#驱动提供对decimal类型的支持
- Java驱动Decimal类型提供比较接口
- PHP驱动提供对PHP 5.6.x的支持
- C++驱动提供连接池能力
- snapshot collectionspace/snapshot collection增加commit相关信息
- snapshot database增加complete lsn和lsn队列信息

**主要特性：**

- 提供数据库元数据操作的一致性
- CM提供配置动态生效能力
- CM提供本地和远程的节点管控能力
- CM增强System、File和Cmd对象并实现远程能力
- CM实现通过配置参数来控制对节点异常的自动重启功能
- SequoiaDB提供同步日志归档能力，支持压缩和过期清理
- SequoiaDB提供定时和定量方式的脏页刷盘能力，并实现异常重启时副本间数据校验能力，减少数据的全量恢复
- SequoiaDB提供手动刷盘数据和日志的能力
- 提供匹配条件(matcher)支持函数的能力
- 提供匹配条件(matcher)支持流水线的处理能力，能够对同一字段进行多次匹配和函数运算
- 提供对匹配的记录进行数组展开多条记录和只返回数组匹配项的能力
- 提供SequoiaDB配置参数动态生效能力
- 提供LOB元数据和数据分离的能力
- 提供节点和分区组信息的WEB监控能力
- 提供会话、上下文、事务等资源的WEB监控能力
- 提供主机内存、磁盘等信息的WEB监控能力
- 提供节点和分区组启停等的WEB操控能力
- 基于WEB的安装部署支持导入导出配置能力

**工具优化：**

- SequoiaDB提供归档日志回放工具，支持条件过滤和指定LSN的回放能力
- 导出工具支持导出指定集合和集合空间
- sdb提供getLastErrObj获取引擎的详细错误信息
- 导出工具提供kicknull，将null字段转为空字段

**性能优化：**

- C#驱动提供LOB读缓存，提升读取性能
- 减少Java驱动LOB写接口内存拷贝，提升写入性能
- 改进LOB缓存的合并算法，提升LOB写入性能和稳定性
- 优化PG SQL大表inner join的查询性能

**解决重要Bug：**

- 并发创建删除集合，报-10系统错误
- 执行sql命令，not field is null 没有正确起作用
- 在事务中执行snapshot(10)导致sdbshell coredump
- 执行cl.find({$or:[1,{a:2}]})，导致coord崩溃重启
- 切分表上对lob作读写操作时w=2且组内两个备节点异常重启，读取lob失败
- 查询条件为主表切分字段的边界值查不到结果
- 数据节点在正常操作时由boost异步通信连接断开发生coredump
- 集群模式下多次执行事务commit或rollback报-196错误
- PG SQL用in操作符包含多字段查询时报错
- 事务内更新某条记录的操作没有实际修改记录时，事务提交或回滚后，其它事务不能获取该记录的锁
- 正常关机或重启OS，SequoiaDB未正常退出
- 并发Upsert过程中导致同步日志错误
- cs中存在大lob文件，执行备份命令卡住
- 在开启lob缓存下，dropCS和停节点时出现死循环
- SequoiaDB中包含LZW压缩的集合，异常重启时重组阶段报-304，无法启动
- 集群所在系统集体掉电，重启后发现复制组内节点有重组失败的情况，且发生了严重数据丢失
- 插入lob执行切分，再次切分时设置切分范围有冲突，切分任务创建成功
