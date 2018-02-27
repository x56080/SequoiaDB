##所属集合空间##

SYSCAT

##概念##

SYSCAT.SYSNODES 集合中包含了该集群中所有的节点与复制组信息。每个复制组保存为一个文档。

每个文档包含以下字段：

| 字段名      | 类型   | 描述                                              |
|-------------|--------|---------------------------------------------------|
| GroupName   | 字符串 | 复制组名。                                        |
| GroupID     | 整数   | 复制组 ID，该 ID 在集群中唯一。                   |
| PrimaryNode | 整数   | 该复制组内主节点 ID。                             |
| Role        | 整数   | 复制组角色，可以为：<br> - 0：数据节点 <br> - 2：编目节点 |
| Status      | 整数   | - 1：已激活复制组<br> - 0：未激活复制组<br> - 不存在：未激活复制组  |
| Version     | 整数   |  版本号，由1起始，任何对该复制组的操作均会对其+1。|
| Group       | 数组   |  复制组中节点信息，见下表：                       |


复制组中如果存在一个以上节点，则每个节点作为一个对象存放在 Group字段数组中，每个对象的信息如下：

| 字段名   | 类型   | 描述         |
|----------|--------|--------------|
| HostName | 字符串 | 节点所在的系统名，需要完全匹配该节点所在操作系统中“hostname”命令的输出。                           |
| dbpath   | 字符串 | 数据库路径，为节点所在的物理节点中对应的绝对路径。|
| instanceid | 整数 | 节点的实例 ID，用于 --preferedinstance 进行实例选择。 |
| NodeID   | 整数   | 节点 ID，该 ID 在集群中唯一。 |
| Service  | 数组   | 服务名，每个逻辑节点对应4个服务名，每个服务名包括其类型与服务名（可以为端口号或services 文件中的服务名）。类型如下：<br>- 0：直连服务，对应数据库参数 svcname   <br>- 1：复制服务，对应数据库参数 replname   <br>- 2：分区服务，对应数据库参数 shardname   <br>- 3：编目服务，对应数据库参数 catalogname  |

>**Note:**

>- 编目复制组名固定为“SYSCatalogGroup”，复制组ID固定为1。
>- 数据复制组 ID 由1000起始。
>- 数据节点 ID 由1000起始。

##示例##

一个典型的包含单节点的编目复制组为：

```
{
  "Group" :
  [
    {
      "NodeID" : 2,
      "HostName" : "vmsvr1-rhel-x64",
      "Service" :
      [
        { "Type" : 3, "Name" : "11803" },
        { "Type" : 1, "Name" : "11801" },
        { "Type" : 2, "Name" : "11802" },
        { "Type" : 0, "Name" : "11800" }
      ],
      "dbpath" : "/home/sequoiadb/sequoiadb/catalog"
    }
  ],
  "GroupID" : 1,
  "GroupName" : "SYSCatalogGroup",
  "PrimaryNode" : 2,
  "Role" : 2,
  "Version" : 1
}
```

一个典型的包含单节点的数据复制组为：

```
{
  "Group" :
  [
    {
      "dbpath" : "/home/sequoiadb/sequoiadb/data3",
      "HostName" : "vmsvr1-rhel-x64",
      "Service" :
      [
        { "Type" : 0, "Name" : "11820" },
        { "Type" : 1, "Name" : "11821" },
        { "Type" : 2, "Name" : "11822" },
      ],
      "NodeID" : 1001,
      "instanceid" : 10
    }
  ],
  "GroupID" : 1001,
  "GroupName" : "foo1",
  "PrimaryNode" : 1001,
  "Role" : 0,
  "Status" : 1,
  "Version" : 1
}
```
