##描述##

分区组列表 SDB_LIST_GROUPS 列出当前集群中的所有分区信息。

##标示##

SDB_LIST_GROUPS

##字段信息##

| 字段名             | 类型   | 描述                           |
| ------------------ | ------ | ------------------------------ |
| Group.dbpath       | 字符串 | 分区组中节点的数据文件存放路径 |
| Group.Status       | 整型   | 分区组中节点的状态             |
| Group.HostName     | 字符串 | 分区组中节点的主机名           |
| Group.Service.Type | 整型   | 分区组中节点的服务类型<br>- 0：直连服务，对应数据库参数 svcname <br>- 1：复制服务，对应数据库参数 replname <br>- 2：分区服务，对应数据库参数 shardname<br>- 3：编目服务，对应数据库参数 catalogname |
| Group.Service.Name | 字符串 | 分区组中节点的服务名，服务名可以为端口号，或 services 文件中的服务名 |
| Group.NodeID       | 整型   | 分区组中节点的 ID              |
| GroupID            | 整型   | 分区组 ID                      |
| GroupName          | 字符串 | 分区组名称                     |
| PrimaryNode        | 整型   | 主节点 ID                      |
| Role               | 整型   | 分区组角色，可以为：<br>- 0：数据节点<br>- 2：编目节点 |
| Status             | 字符串 | 分区组状态：<br>- 1：已激活分区组<br>- 0：未激活分区组<br>- 不存在：未激活分区组 |
| Version            | 整型   |                                |

##示例##

```lang-javascript
> db.list( SDB_LIST_GROUPS )
{
  "Group":[
    {
      "HostName": "hostname1",
      "Status": 1,
      "dbpath": "/data/disk1/sequoiadb/database/catalog/11860/",
      "Service": [
        {
          "Type": 0,
          "Name": "11800"
        },
        {
          "Type": 1,
          "Name": "11801"
        },
        {
          "Type": 2,
          "Name": "11802"
        },
        {
          "Type": 3,
          "Name": "11803"
        }
      ],
      "NodeID": 1
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "PrimaryNode": 1,
  "Role": 2,
  "Status": 1,
  "Version": 1,
  "_id": {
    "$oid": "51710981d8cb8fbc163d6350"
  }
}
...
```
