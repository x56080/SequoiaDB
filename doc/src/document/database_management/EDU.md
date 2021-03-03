##概念##

引擎调度单元（Engine Dispatchable Unit）是 SequoiaDB 数据库中任务运行的载体，一般来说一个 EDU 意味着一个单独的线程。

每个 EDU 可以用来执行用户的请求，或者执行系统内部的维护任务。

EDU 之间相互独立，不同 EDU 单独负责不同的用户会话。一个用户会话与一个 EDU，在一个数据节点中相互绑定。

每个 EDU 拥有一个进程内唯一的64位整数标示，称作“EDU ID”。

EDU 可以分为用户 EDU 与系统 EDU，分别代表执行用户任务的线程，与执行系统任务的线程。

![引擎调度](database_management/sequoiadb_infrastructure_data.jpg)

##用户EDU##

用户 EDU 为执行用户任务的线程，一般又叫作代理（Agent）线程。

在 SequoiaDB 中，主要存在下列代理线程类型：

| 名称       | 类型      | 描述 |
| ---------- | --------- | ---- |
| Agent      | 代理      | 代理线程负责由 svcname 服务传入的请求，一般来说该请求由用户直接传入。 |
| ShardAgent | 分区代理  | 分区代理线程负责由 shardname 服务传入的请求，一般来说该请求由协调节点传入数据节点。 |
| ReplAgent  | 复制代理  | 复制代理线程负责由 replname 服务传入的请求，一般来说该请求由数据主节点传向从节点，多作用于非协调节点。 |
| HTTPAgent  | HTTP 代理 | HTTP 代理线程负责由 httpname 服务传入的 REST 请求，一般来说该请求由用户直接传入。 |

##系统EDU##

系统 EDU 为系统内部维护数据结构及一致性的线程，一般来说对用户完全透明。

在 SequoiaDB 中，存在但不局限于下列系统 EDU：

| 名称            | 类型             | 描述 |
| --------------- | ---------------- | ---- |
| TCPListener     | 服务监听         | 该线程负责监听 svcname 服务，并启动 Agent 代理线程 |
| HTTPListener    | HTTP 监听        | 该线程负责监听 httpname 服务，并启动 Agent 代理线程 |
| Cluster         | 集群管理         | 集群管理线程用于维护集群的基本框架，启动 ReplReader 与 ShardReader 线程 |
| ReplReader      | 复制监听         | 复制监听线程负责由 replname 服务传入的请求，并启动 ReplAgent 代理线程 |
| ShardReader     | 分区监听         | 分区监听线程负责由 shardname 服务传入的请求，并启动 ShardAgent 代理线程 |
| LogWriter       | 日志写           | 日志写线程用于将日志缓冲区中的数据写入日志文件 |
| WindowsListener | Windows 事件监听 | Windows 环境特有，用于监听 Windows 中 SequoiaDB 定义事件 |
| Task            | 后台任务处理     | 后台任务处理线程，一般来说用于处理后台任务请求，例如：[数据切分](basic_operation/sharding/data_split.md) |
| CatalogManager  | 编目控制         | 编目控制线程用于处理编目节点内部元数据相关的请求 |
| CatalogNetwork  | 编目网络监听     | 编目网络监听线程用于监听编目服务 catalogname 下的请求 |
| CoordNetwork    | 协调网络监听     | 协调网络监听线程用于监听分区的请求 |

##监控##

用户可以使用 [会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md) 监控系统与用户 EDU。
