SequoiaDB 数据库是一款新型企业级分布式非关系型数据库，帮助企业用户降低 IT 成本，并对大数据的存储与分析提供了一个坚实，可靠，高效与灵活的底层平台。

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
- cache相关优化
- 索引性能优化
- 优化LOB并发回放的机制
- 优化存储空间分配策略

**解决重要bug：**

- 在磁盘空间满时备节点出现日志损坏并导致全量同步
- 备节点磁盘满时创建cs/cl与读写LOB并发在回滚删除LOB日志失败时该备节点宕机
- 插入相同数据并多次停启主节点导致主备数据不一致
- 开启数据压缩时创建字典的并发操作存在问题
- 集群整体掉电导致数据节点rebuild失败无法启动
- 并发增删改操作过程中集群掉电故障恢复后数据组主节点rebuid过程中频繁重启

**其它优化：**

- OM部署业务完成后发现远程主机的database用户权限为root
- Java驱动bson的Binary类中缺少equals()方法
- 编目节点只能detach而不能attach
- sequoiasql_oltp用in操作符包含多字段查询时报错
- Java驱动构造异常指定错误码不存在时getErrorCode()返回为0
- 在使用systemd的系统上卸载SequoiaDB失败
- 使用sdb shell脚本查询1MB左右的二进制数据时报错
- 在SUSE12使用SAC部署业务时重复选择磁盘
- Java驱动timestamp字符串格式错误时解析不报错而返回当前系统时间
- SAC添加业务时支持强制输入不存在的配置项
- Java驱动BSONObject转成JSON后的String括号前后空格不一致
- 升级安装时等待端口时间从6s延长到30s
- SAC安装业务失败
- Java驱动中集合不存在时CollectionSpace.getCollection()返回null
- Java驱动构造SequoiaDB传入的用户密码错误时报错信息中语法有误
- Python驱动调用collection.create_id_index()/collection.drop_id_index()接口报错
- 开启trace后内存泄露
- Python驱动collection.save()接口指定_id为非objectID类型时报错
- Python驱动collection.get_query_meta()接口的参数问题
- C驱动socket的recv返回EAGAIN/EWOULDBLOCK时未重试
- Python驱动的timestamp类型问题
- Python驱动的binary数据类型问题
- 执行queryAndUpdate()未执行cursor.next()更新数据成功
- Python驱动replicagroup.is_catalog()接口报const.TRUE未定义的错误
- Python驱动中调用replicanode.connect()没有返回连接对象
- Python驱动集群管理接口返回值问题
- Python驱动client.list_tasks()接口错误
- preferinstance默认值从A改成M
- Java驱动BSON中Long值比对结果错误
- 安装sequoiasql-oltp的run包会导致之前的环境变量被覆盖
- sequoiasql-oltp安装包在静默模式支持覆盖安装
- sdbinspect工具将字段顺序不一致的同一条记录检测为不一致的记录
- Java驱动ReplicaGroup.getMaster()接口获取信息为null不抛异常
- Collection.listLobs()卡死
- rtnPredicate内存泄露
- sdb shell多次执行traceFmt()产生错误的数据
- catch错误时打印日志cond参数写错
- 查询集合时STRINGOUT功能存在问题
- 反复停启数据主节点导致主节点dms元数据totalRecords与实际数据个数不一致
- 连接coord节点停止coord节点组失败
- 日志归档开启压缩时节点宕机
- 使用默认用户、密码和端口创建ssh连接失败
- 超出一定范围的INT64类型数值与DOUBLE类型数值比较结果不正确
- SAC在中文语言环境中报错
- SAC上创建业务时在修改业务步骤删除节点之后打开节点编辑窗口信息错误
- PHP驱动install的接口使用字符串参数可能会异常退出
- SAC添加主机失败提示信息不足
- SAC添加主机过程中，点击上一步会导致主机添加失败
- 并发执行事务过程中执行事务快照导致节点宕机
- Python驱动调用collection的LOB相关的接口时传入错误的str类型的oid导致挂掉
- 插入大量LOB数据时删除集合空间导致节点崩溃
- 当集合空间LobPageSize不为默认值时在节点重启后该集合空间的Cache页大小不正确
- Java驱动BasicBSONObject实现hashCode()方法
- PHP驱动不能使用SSL连接
- 扩展页面的任务名字中包含集合空间名
- PHP驱动直接调用SecureSdb返回值为字符串而不是数组
- 并发detach/attach节点时报-158错误
- 查询连接较多时数据节点卡住
- 节点故障重启后rebuild过程中死锁
- 部分代码中trace出入口不配对
- Java驱动JSON.parse()解析日期时对时区处理的问题
- PHP驱动snapshot定义的类型跟其它驱动不统一
- detach子表时内存泄露
- 文件句柄不足时创建集合报错时错误码不准确
- 协调节点上执行命令时内存泄漏
- forceStepUp()可能会导致节点宕机
- Java驱动无参构造BSONTimestamp对象后调用toString报空指针异常
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
