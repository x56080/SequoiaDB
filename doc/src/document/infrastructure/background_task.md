后台任务是 SequoiaDB 中的一种特殊任务类型，一般用于将特定用户操作置于后台异步执行。在 [会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md) 中，后台任务的类型（Type）为 "Task"。

后台任务类型列表：

| 任务名             |   描述   |
|--------------------|----------|
| Restore            |  数据库恢复任务，用于根据日志文件回滚恢复数据库。|
| CreateIndex        |  建立索引任务，用于后台建立索引，多用于备节点重做主节点的建立索引操作日志。|
| DropIndex          |  删除索引任务，用于后台删除索引，多用于备节点重做主节点的删除索引操作日志。|
| CleanUp            |  数据清理任务，多用于数据切分后，在源数据节点删除被切分数据。|
| Load               |  数据加载任务，数据库重新启动后对数据进行加载。 |
| Rebuild            |  数据重建任务，数据库重新启动后数据损坏时进行数据重建。 |
| DictionaryCreator  |  创建数据字典任务，进行数据压缩时用于后台创建数据字典。 |
| DmsCheck           |  数据校验任务，用于检查数据文件和编目元数据一致性。 |
| OptPlanClear       |  访问计划缓存清理任务，用于清理过时的访问计划缓存。 |
| CACHE-JOB          |  数据缓存管理任务，对数据缓存进行管理（目前只对大对象 LOB 数据缓存进行管理）。可以使用 SequoiaDB 的 --maxcachesize 控制数据缓存管理任务的数量。 |
| DATASYNC-JOB       |  脏页清除任务，用于异步将未写入磁盘的脏页刷入磁盘。可以使用 SequoiaDB 的 --maxsyncjob 控制脏页清除任务数量。 |
| PAGEMAPPING-JOB    |  索引数据页管理任务，用于异步管理索引页的分裂等，以提高索引页的使用效率。 |
| Job[Prefetch]      |  预取任务，用于在等待客户端接收下一个操作请求时，在后台执行用户接下来可能发生的操作。可以使用 SequoiaDB 的 -maxprefpool 控制最大预取任务的数量。 |
| Job[ExtendSegment] |  扩展集合空间文件任务，用于当集合空间空闲数据页小于特定阀值后，由后台启动异步扩充集合空间。|
| Job[ReplSync]      |  并发同步日志任务，负责并发同步日志。可以使用 SequoiaDB 的 --maxreplsync 参数控制并发同步任务的数量。 |
