##描述##

存储单元列表 SDB_LIST_STORAGEUNITS 列出当前数据库节点的全部存储单元信息。

##标示##

SDB_LIST_STORAGEUNITS

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
> db.list( SDB_LIST_STORAGEUNITS )
{
  "NodeName": "r520-8:11890",
  "Name": "foo1",
  "UniqueID": 61,
  "ID": 4095,
  "LogicalID": 0,
  "PageSize": 4096,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 1,
  "CollectionHWM": 1,
  "Size": 172032000
}
{
  "NodeName": "r520-8:11890",
  "Name": "foo2",
  "UniqueID": 62,
  "ID": 4094,
  "LogicalID": 1,
  "PageSize": 4096,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 2,
  "CollectionHWM": 3,
  "Size": 172032000
}
...
```
