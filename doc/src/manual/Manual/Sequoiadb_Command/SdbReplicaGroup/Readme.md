SdbReplicaGroup 类主要用于操作复制组和获取复制组相关的信息，包含的函数如下：

| 名称 | 描述 |
|------|------|
| attachNode() | 将不属于任何复制组的节点加入当前复制组 |
| createNode() | 在当前复制组中创建节点 |
| detachNode() | 分离当前复制组内的一个节点 |
| getDetailObj() | 获取复制组的信息 |
| getMaster() | 获取当前复制组的主节点 |
| getNode() | 获取当前复制组的指定节点 |
| getSlave() | 获取当前复制组的备节点 |
| reelect() | 在当前复制组中重新选举 |
| removeNode() | 删除当前复制组中的指定节点 |
| start() | 启动当前复制组 |
| stop() | 停止当前复制组 |