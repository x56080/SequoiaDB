##介绍##

**命令位置参数** 是用于控制命令执行的位置信息，包括“命令是否在全局运行还是在本地运行”、“命令运行的分区组”、“命令运行的节点”、“节点的选取方式”、“命令运行的角色”等。该参数以Json对象作为命令的参数传入，并且只在“协调节点”生效。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| Global | bool     | 是否在全局执行 | 否 |
| GroupID | int 或数组 | 分区组ID | 否 |
| GroupName | string 或数组 | 分区组名 | 否 |
| NodeID | int 或数组 | 节点ID | 否 |
| HostName | string 或数组 | 节点的主机名称 | 否 |
| svcname | string 或数组 | 节点的服务名 | 否 |
| NodeSelect | string | 在未指定节点时分区组的节点选择策略，取值：<br> *all*: 选择该组所有节点<br>*master(primary)*: 选择该组主节点<br>*any*: 选择该组任意节点<br>*secondary*: 选择该组任意备节点  | 否 |
| Role | string 或数组 | 指定命令运行的节点角色，取值：<br> *data*: 数据节点<br> *catalog*: 编目节点<br> *coord*: 协调节点<br> *all*: 所有节点 | 否 |
| RawData | bool | 是否返回原始数据，仅对 [list](reference/Sequoiadb_command/Sdb/list.md) 或 [snapshot](reference/Sequoiadb_command/Sdb/snapshot.md) 命令生效，<br>为 true 则返回各节点的原始数据，不在协调节点进行聚集处理 | 否 |


> **Note:**
>
> * 当设置了GroupID, GroupName, NodeID, HostName 或 svcname时，Global取值被忽略，在指定的分区组或节点上执行。
> * GroupID、GroupName：指定分区组过滤条件，缺省指所有分区组；GroupID和GroupName为或的关系，如：{GroupID:1001, GroupName:"db1"}，那么分区组1001和db1都是执行的分区组。
> * NodeID、HostName、svcname：指定分区组中节点过滤条件，对于查询命令，缺省值为该组所有节点，对于操作命令，缺省值为该组主节点。上述字段为与的关系，如 {NodeID:1001, svcname:'11810'}，如果节点1001的svcname不为11810，则节点为空。
> * Groups: 为了兼容之前的命令而保留，与GroupName作用相同，不推荐使用。
> * Role: 当按分区组过滤时，该参数不生效。

