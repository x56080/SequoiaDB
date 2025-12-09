[^_^]:
    复制组熔断机制

在 SequoiaDB 巨杉数据库的[复制组][replicaSet]中，所有写入和更新操作均在主节点进行，各备节点通过日志文件与主节点进行数据同步。在节点磁盘空间满、网络异常或其他原因造成不能同步数据时，可能会导致主节点的写操作阻塞。为此，SequoiaDB 提供了容错机制，在发生上述异常的情况下，自动采用降级、限流或剔除节点的方式避免写操作阻塞，以保证业务的正常运行。

##容错诊断##

在进行容错处理前，数据库需要进行容错诊断，确认是否发生指定类型的故障。用户可通过容错掩码（对应参数 [ftmask][config]）指定需进行容错处理的故障类型。

在诊断过程中，数据库将结合故障确认的时间周期（对应参数 [ftconfirmperiod][config]）和故障确认的故障比阈值（对应参数 [ftconfirmratio][config]）确认是否发生故障。如果在时间周期内，故障比阈值超过指定值，则认为故障发生。以参数 ftconfirmperiod=30s、ftconfirmratio=80% 为例，容错数据采样间隔时间固定为 3s，则指定时间周期内将采样 10 次。由于指定的故障比阈值为 80%，10 次采样中如果超过 8 次显示数据异常，数据库会认为故障已发生。

下述将根据参数 ftmask 的取值介绍各类故障的异常确认机制。

###NOSPC###

当 ftmask 取值包含"NOSPC"，表示处理因磁盘空间不足而发生的故障。数据库将在故障确认周期内多次检查磁盘空间的使用情况。如果采样的数据显示磁盘空间不足，则认为检测到异常。当采样中检测到异常的比率超过故障比阈值，则发生故障。

###DEADSYNC###

当 ftmask 取值包含"DEADSYNC"，表示处理因节点数据不同步而发生的故障。数据库将在故障确认周期内多次检查当前节点LSN是否与主节点存在差距且自身LSN停止增涨，如果检测符合上述情况，则认为检测到异常。当采样中检测到异常的比率超过故障比阈值，则发生故障。

###SLOWNODE###

当 ftmask 取值包含"SLOWNODE"，表示处理因节点数据同步过慢而发生的故障。数据库将在故障确认周期内多次检查当前节点与主节点的日志差阈值（对应参数 [ftslownodethreshold][config]）和周期间自身日志堆积增量值（对应参数 [ftslownodeincrement][config]）是否在正常范围内。如果采样的数据显示备节点与主节点的日志差值超过指定值，且备节点的日志堆积增量值超过指定值，则认为检测到异常。当采样中检测到异常的比率超过故障比阈值，则发生故障。

###DISKFAULT###

当 ftmask 取值包含"DISKFAULT"，表示处理因节点磁盘慢或磁盘异常而发生的故障。数据库将在故障确认周期内多次检查当前节点各磁盘目录读写是否正常以及读写操作耗时（对应参数 [ftdiskslowthreshold][config]）和周期间读写耗时增量值（对应参数 [ftdiskslowincrement][config]）是否在正常范围内。如果采样的数据显示读写耗时超过指定值，且读写耗时增量值超过指定值，则认为检测到异常。当采样中检测到异常的比率超过故障比阈值，则发生故障。

###NONE###

当 ftmask 取值为"NONE"，表示关闭所有容错处理。数据库将不对任何操作进行报错，易触发全量同步。

###ALL###

当 ftmask 取值为"ALL"，表示开启所有容错处理。

##容错处理##

容错诊断后，容错确认信息会通过[心跳][election]在主备节点间传递。用户可以通过[数据库快照][SDB_SNAP_DATABASE]的字段 FTStatus 确认容错状态。各节点确认容错状态后，数据库将根据容错级别对不同 [ReplSize][createCL] 取值的写操作进行报错或降级处理。用户可通过参数 [ftlevel][config] 指定容错级别。

下述将根据参数 ftlevel 的取值介绍不同容错级别的容错处理机制。

###熔断###

当 ftlevel 的取值为 1，数据库将严格按照 ReplSize 的取值进行写操作，对于不能满足 ReplSize 规则的写操作均报错处理。以三副本为例，当指定集合的 ReplSize 为 3 时，如果复制组中一个节点故障，写操作将报错。

在该级别下，如果指定掩码的异常能在熔断超时时间内恢复，则不对异常节点进行熔断处理。用户可通过参数 [ftfusingtimeout][config] 指定熔断超时时间

###半容错###

当 ftlevel 的取值为 2，数据库对 ReplSize 为 -1(Alive nodes)、-2(Major nodes)、-3(Alive major nodes) 的写操作进行降级处理，对于降级后依旧不能满足 ReplSize 规则的写操作则进行报错处理。以三副本为例，当指定集合的 ReplSize 为 -1 时，如果复制组中一个节点故障，写操作将被降级为 2，表示写操作只在两个节点中进行。

###全容错###

当 ftlevel 的取值为 3，数据库将对 ReplSize 为任意取值的写操作都尝试进行降级处理，对于降级后依旧不能满足 ReplSize 规则的写操作则进行报错处理。以三副本为例，当指定集合的 ReplSize 为 3 时，如果复制组中一个节点故障，写操作将被降级为 2，表示写操作只在两个节点中进行。






[^_^]:
    本文使用的所有引用及链接
[replicaSet]:manual/Distributed_Engine/Architecture/Replication/Readme.md
[config]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[election]:manual/Distributed_Engine/Architecture/Replication/election.md
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md