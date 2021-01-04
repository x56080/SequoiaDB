
SYSCAT.SYSTASKS 集合中包含了该集群中正在运行的后台任务信息，每个任务保存为一个文档。

每个文档包含以下字段：

| 字段名          | 类型   | 描述                                              |
|-----------------|--------|---------------------------------------------------|
| JobType         | number | 任务类型，取值如下：<br>   0：数据切分            |
| Status          | number | 任务状态，取值如下：<br> 0：准备<br> 1：运行<br>  2：暂停<br>  3：取消<br>  4：变更元数据<br> 9：完成<br>  不存在：未激活复制组 |
| CollectionSpace | string |  集合空间名                                       |
| Collection      | string |  集合名                                           |

对于数据切分操作，每个文档还存在以下字段：

| 字段名     |   类型  |   描述                |
|------------|---------|-----------------------|
| SourceName |  string |  源分区所在复制组名   |
| TargetName |  string |  目标分区所在复制组名 |
| SourceID   |  number |  源分区所在复制组 ID  |
| TargetID   |  number |  目标分区所在复制组 ID|
| SplitValue |  object |  数据分区键           |

>  **Note:**
>
>  目前 SYSCAT.SYSTASKS 仅有数据切分任务。
