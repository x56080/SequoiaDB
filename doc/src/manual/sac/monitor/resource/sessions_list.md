**会话** 页面可以查看存储集群所有会话。

![会话列表](sac/monitor/resource/sessions_list_1.png)

该表格列出了存储集群的所有[会话快照](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS.md)信息。

  - 点击 **SessionID** 可以查看所选会话的详细信息。
  - 需要选择显示哪些字段，可以通过点击表格上方的 **选择显示列** 按钮来选择。
  - 需要排序时，可以通过点击表格表头来根据字段进行排序。
  - 需要搜索某个字段时，可以在所在字段上方的输入框输入关键字进行搜索。

> **Note：**  
> 1. 表格中 **Classify** 列是为了更好的分类而添加显示的字段，并不是会话快照自带的字段信息。  
> 2. 会话快照默认显示非 **Idle** 状态和外部的会话( Type 是 Agent、ShardAgent、ReplAgent、HTTPAgent 的会话)，可通过字段下方的筛选框选择显示所有会话。  
> 3. 会话快照对应的字段说明可以通过[会话快照](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS.md)查看。