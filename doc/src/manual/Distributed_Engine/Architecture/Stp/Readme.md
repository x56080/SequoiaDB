时间序列协议（STP，Sequence Time Protocol）是 SequoiaDB 内部逻辑时间同步的协议：

- STP 维护的是逻辑时间，提供逻辑时钟服务
- 在 SequoiaDB 中逻辑时间戳主要用于全局事务处理
- 在 SequoiaDB 中以机器为单位维护逻辑时间戳

> **Note:**
>
> * 逻辑时间是 SequoiaDB 内部用于表示时间但区别于实际时间的逻辑时间戳，可参考[逻辑时间][logicaltime]。
> * STP 需要部署在 SequoiaDB 集群的每个机器中以提供逻辑时钟服务。
> * 全局逻辑时钟服务可以提供全局事务的支持，可参考[事务操作][configurations]。

STP 节点包含以下两类角色（Role）：

- STP server：可以作为同步源的时间节点
- STP client：只能向同步源同步时间的时间节点

> **Note:**
>
>  详细说明可参考 [STP 的配置参数][stp]。


[^_^]:
    本文使用的所有引用及链接
[logicaltime]:manual/Distributed_Engine/Architecture/Stp/logicaltime.md
[configurations]:manual/Distributed_Engine/Architecture/Transactions/configurations.md
[stp]:manual/Distributed_Engine/Architecture/Stp/Tools/stp.md#参数说明
