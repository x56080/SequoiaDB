节点信息有4个子页面，分别是：
  
  - 节点信息，可以查看所选节点的运行状态、详细信息、及节点增删改查操作的实时速率。
  - 节点会话，查看所选节点的会话快照。
  - 节点上下文，查看所选节点的上下文快照。
  - 节点图表，查看所选节点的会话数量、上下文数量、事务数量的实时图表。

###节点信息###

![节点信息](sac/monitor/node/node_info_1.png)

###节点会话###

![节点会话](sac/monitor/node/node_info_2.png)

点击 **SessionID** 可以查看所选会话的详细信息。

![节点会话](sac/monitor/node/node_info_3.png)

> **Note:**  
> 1. 点击 **选择显示列** 可以选择显示哪些字段。  
> 2. 表格中 **Classify 列** 是为了更好的分类，并不是会话快照的字段。  
> 3. 会话快照默认显示非 **Idle** 状态和外部的会话("Type"为"Agent"、"ShardAgent"、"ReplAgent"、"HTTPAgent"的会话)，可以自定义过滤。  
> 4. 会话快照对应的字段说明可以在 [会话快照](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS.md) 查看。

###上下文###

![节点上下文](sac/monitor/node/node_info_4.png)

点击 **CntextID** 可以查看所选上下文的详细信息。

![节点上下文](sac/monitor/node/node_info_5.png)

> **Note:**  
> 1. 点击 **选择显示列** 可以选择显示哪些字段。  
> 2. 上下文快照对应的字段说明可以在 [上下文快照](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_CONTEXTS.md) 查看。

###图表###

页面显示所选节点的会话、上下文和事务的实时数量图表。

![节点图表](sac/monitor/node/node_info_6.png)