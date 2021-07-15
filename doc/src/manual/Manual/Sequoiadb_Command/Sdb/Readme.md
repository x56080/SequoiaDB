Sdb 类主要用于连接和操作 SequoiaDB 巨杉数据库，包含的函数函数如下：

| 名称 | 描述 |
|------|------|
| Sdb() | SequoiaDB 连接对象 |
| analyze() | 收集统计信息 |
| backup() | 备份数据库 |
| cancelTask() | 取消任务 |
| close() | 关闭数据库连接 |
| createCataRG() | 新建编目复制组 |
| createCoordRG() | 创建协调复制组 |
| createCS() | 创建集合空间 |
| createDataSource() | 创建数据源 |
| createDomain() | 创建域 |
| createProcedure() | 创建存储过程 |
| createRG() | 新建复制组 |
| createSequence() | 创建序列对象 |
| createSpareRG()| 创建热备组 |
| createUsr() | 创建数据库用户 |
| dropCS() | 删除一个已存在的集合空间 |
| dropDataSource() | 删除数据源 |
| dropDomain() | 删除域 |
| dropSequence() | 删除指定的序列 |
| dropUsr() | 删除数据库用户 |
| eval() | 调用存储过程 |
| exec() | 执行 SQL 的 select 语句 |
| execUpdate() | 执行除 select 以外的 SQL 语句 |
| flushConfigure() | 将节点内存中的配置刷盘至配置文件 |
| forceSession() | 终止指定会话的当前操作 |
| forceStepUp() | 强制将备节点升级为主节点 |
| getCataRG() | 获取编目复制组的引用 |
| getCoordRG() | 获取协调复制组的引用 |
| getCS() | 获取指定集合空间 |
| getDataSource() | 获取数据源的引用 |
| getDomain() | 获取指定域 |
| getRG() | 获取指定复制组 |
| getSequence() | 获取指定的序列对象 |
| getSessionAttr() | 获取会话属性 |
| getSpareRG() | 获取备份组的引用 |
| invalidateCache() | 清除节点的缓存信息 |
| list() | 获取列表 |
| listBackup() | 枚举数据库备份 |
| listCollections() | 枚举集合信息 |
| listCollectionSpaces() | 枚举集合空间信息 |
| listDataSources() | 查看数据源的元数据信息 |
| listDomains() | 枚举用户创建的域 |
| listProcedures() | 枚举存储过程 |
| listReplicaGroups() | 枚举复制组信息 |
| listSequences() | 枚举序列信息 |
| listTasks() | 枚举后台任务 |
| loadCS() | 加载集合空间到内存 |
| removeBackup() | 删除数据库备份 |
| removeCataRG() | 删除编目复制组 |
| removeCoordRG() | 删除数据库中的协调复制组 |
| removeSpareRG() | 删除数据库中的 SYSSpare 组 |
| removeProcedure() | 删除指定的函数名 |
| removeRG() | 删除复制组 |
| renameCS() | 集合空间改名 |
| renameSequence() | 修改序列名 |
| resetSnapshot() | 重置快照 |
| reloadConf() | 重新加载配置文件 |
| deleteConf() | 删除配置 |
| setSessionAttr() | 设置会话属性 |
| snapshot() | 获取快照 |
| startRG() | 启动复制组 |
| stopRG() | 停止复制组 |
| sync() | 持久化数据和日志到磁盘 |
| setPDLevel() | 动态设置节点的诊断日志级别 |
| traceOff() | 关闭数据库引擎跟踪功能 |
| traceOn() | 开启数据库引擎跟踪功能 |
| traceResume() | 重新开启断点跟踪程序 |
| traceStatus() | 查看当前程序跟踪的状态 |
| transBegin() | 开启事务 |
| transCommit() | 事务提交 |
| transRollback() | 事务回滚 |
| unloadCS() | 卸载集合空间 |
| updateConf() | 更新节点配置 |
| waitTasks() | 同步等待指定任务结束或取消 |
