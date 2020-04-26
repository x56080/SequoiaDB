##概述##

**命令位置参数** 是用于控制命令执行的位置信息，包括命令是否在全局运行还是在本地运行、命令运行的分区组、命令运行的节点、节点的选取方式、命令运行的角色等。该参数以Json对象作为命令的参数传入，并且只在协调节点生效。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| Global | Bool     | 是否在全局执行 | 否 |
| GroupID | Int32/Array | 分区组ID | 否 |
| GroupName | String/Array | 分区组名 | 否 |
| NodeID | Int16/Array | 节点ID | 否 |
| HostName | String/Array | 节点的主机名称 | 否 |
| ServiceName | String/Array | 节点的服务名 | 否 |
| svcname | String/Array | 节点的服务名 | 否 |
| NodeName | String/Array | 节点名称，格式为 \<HostName\>:\<svcname1\>[:svcname2...]，<br> 如：sdbserver:11820:11830 | 否 |
| NodeSelect | String | 在未指定节点时分区组的节点选择策略，取值：<br> *all*: 选择该组所有节点<br>*master/primary*: 选择该组主节点<br>*any*: 选择该组任意节点<br>*secondary*: 选择该组任意备节点  | 否 |
| Role | String/Array | 指定命令运行的节点角色，取值：<br> *data*: 数据节点<br> *catalog*: 编目节点<br> *coord*: 协调节点<br> *all*: 所有节点 | 否 |
| RawData | Bool | 是否返回原始数据，仅对 [list](reference/Sequoiadb_command/Sdb/list.md) 或 [snapshot](reference/Sequoiadb_command/Sdb/snapshot.md) 命令生效，<br>为 true 则返回各节点的原始数据，不在协调节点进行聚集处理 | 否 |
| InstanceID | Int32/Array | 节点的实例 ID（数据节点通过的配置项 instanceid 指定）<br>有效取值范围：1 - 255 <br>指定 InstanceID 时仅选取数据节点 | 否 |


> **Note:**
>
> * 当设置了GroupID, GroupName, NodeID, HostName, ServiceName或NodeName时，Global取值被忽略，在指定的分区组或节点上执行。
> * GroupID、GroupName：指定分区组过滤条件，缺省指所有分区组；GroupID和GroupName为或的关系，如：{GroupID:1001, GroupName:"db1"}，那么分区组1001和db1都是执行的分区组。
> * NodeID、HostName、ServiceName、NodeName：指定分区组中节点过滤条件，对于查询命令，缺省值为该组所有节点，对于操作命令，缺省值为该组主节点。上述字段为与的关系，如 {NodeID:1001, ServiceName:'11810'}，如果节点1001的ServiceName不为11810，则节点为空。
> * Groups: 为了兼容之前的命令而保留，与GroupName作用相同，不推荐使用。
> * svcname：与ServiceName参数功能相同，都表示设置节点服务名。
> * instanceid：与 InstanceID 参数功能相同，都表示设置节点的实例 ID。
