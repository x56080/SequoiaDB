Stp 类主要用于操作时间序列服务，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [Stp][Stp] | STP 服务进程对象 |
| [getTime][getTime] | 获取 STP 节点当前的逻辑时间 |
| [getTimeUS][getTimeUS] | 获取 STP 节点当前的逻辑时间 |
| [getConf][getConf] | 获取 STP 节点的配置 |
| [getMeta][getMeta] | 获取 STP 节点的元数据信息 |
| [getServers][getServers] | 获取 STP 节点所同步的 server 组信息 |
| [getSyncClients][getSyncClients] | 获取 STP 节点所在集群的时间同步信息 |
| [getSyncStatus][getSyncStatus] | 获取 STP 节点与当前同步源的同步信息 |
| [getSyncHistory][getSyncHistory] | 获取 STP 节点与各同步源的历史时间同步信息 |
| [reelect][reelect] | 在 STP 节点所在的 server 组中重新选主 |
| [stop][stop] | 停止 STP 服务进程 |
| [updateConf][updateConf] | 修改 STP 节点的配置 |

[^_^]:
     本文使用的所有引用及链接
[Stp]:manual/Manual/Sequoiadb_Command/Stp/Stp.md
[getTime]:manual/Manual/Sequoiadb_Command/Stp/getTime.md
[getTimeUS]:manual/Manual/Sequoiadb_Command/Stp/getTimeUS.md
[getConf]:manual/Manual/Sequoiadb_Command/Stp/getConf.md
[getMeta]:manual/Manual/Sequoiadb_Command/Stp/getMeta.md
[getServers]:manual/Manual/Sequoiadb_Command/Stp/getServers.md
[getSyncClients]:manual/Manual/Sequoiadb_Command/Stp/getSyncClients.md
[getSyncStatus]:manual/Manual/Sequoiadb_Command/Stp/getSyncStatus.md
[getSyncHistory]:manual/Manual/Sequoiadb_Command/Stp/getSyncHistory.md
[reelect]:manual/Manual/Sequoiadb_Command/Stp/reelect.md
[stop]:manual/Manual/Sequoiadb_Command/Stp/stop.md
[updateConf]:manual/Manual/Sequoiadb_Command/Stp/updateConf.md