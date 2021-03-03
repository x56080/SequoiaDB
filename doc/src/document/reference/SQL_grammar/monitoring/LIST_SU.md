##描述##

存储单元列表 $LIST_SU 列出当前数据库节点的全部存储单元信息。

##标示##

$LIST_SU

>   **Note:**
>
>   只能在数据节点和编目节点执行。

##字段信息##

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| NodeName       | 字符串 | 所在的节点名                |
| Name           | 字符串 | 集合空间名                  |
| UniqueID       | 整型   | 集合空间的UniqueID，在集群上全局唯一 |
| ID             | 整型   | 该集合空间 ID               |
| LogicalID      | 字符串 | 集合空间逻辑 ID，为递增顺序 |
| PageSize       | 整型   | 集合空间数据页大小          |
| LobPageSize    | 整型   | 集合空间大对象页大小        |
| Sequence       | 整型   | 序列号，当前版本中为 1      |
| NumCollections | 整型   | 集合空间下的集合个数        |
| CollectionHWM  | 整型   | 集合高水位，一般来说意味着该集合空间中总共创建过的集合数量（包括被删除的集合） |
| Size           | 长整型 | 存储单元大小（字节）        |

##示例##

```lang-javascript
> db.exec( "select * from $LIST_SU" )
{
  "NodeName": "hostname:30000",
  "Name": "SYSAUTH",
  "UniqueID": 0,
  "ID": 5,
  "LogicalID": 5,
  "PageSize": 65536,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 1,
  "CollectionHWM": 1,
  "Size": 306315264
}
{
  "NodeName": "hostname:30000",
  "Name": "SYSCAT",
  "UniqueID": 0,
  "ID": 1,
  "LogicalID": 1,
  "PageSize": 65536,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 6,
  "CollectionHWM": 6,
  "Size": 306315264
}
...
```
