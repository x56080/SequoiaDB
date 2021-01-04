
##描述##

集合空间快照 $SNAPSHOT_CS 列出当前数据库节点中所有的集合空间，每个集合空间为一条记录。

##标识##

$SNAPSHOT_CS

##字段信息##

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| NodeName        | string     | 集合空间所属节点名，格式为<主机名>:<端口号>         |
| GroupName       | string     | 集合空间所属复制组名                         |
| Name            | string     | 集合空间名                                   |
| UniqueID        | int32      | 集合空间的 UniqueID，在集群上全局唯一         |
| ID              | int32      | 集合空间在节点上的物理 ID                     |
| LogicalID       | int32      | 集合空间在节点上的逻辑 ID                     |
| Collection.Name | string     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | int64 | 集合空间所包含的集合的UniqueID               |
| PageSize        | int32      | 集合空间数据页大小，单位为字节             |
| LobPageSize     | int32      | 集合空间大对象数据页大小，单位为字节       |
| MaxCapacitySize | int64     | 集合空间的最大容量上限，单位为字节         |
| MaxDataCapSize  | int64     | 集合空间数据文件最大容量上限，单位为字节   |
| MaxIndexCapSize | int64     | 集合空间索引文件最大容量上限，单位为字节   |
| MaxLobCapSize   | int64     | 集合空间大对象文件最大容量上限，单位为字节 |
| NumCollections  | int32      | 集合数量                                     |
| TotalRecords    | int32      | 集合空间的记录总数                           |
| TotalSize       | int64    | 集合空间的总大小，单位为字节               |
| FreeSize        | int64     | 集合空间的空闲大小，单位为字节             |
| TotalDataSize   | int64     | 集合空间数据文件总大小，单位为字节         |
| FreeDataSize    | int64     | 集合空间数据文件空闲空间大小，单位为字节   |
| TotalIndexSize  | int64     | 集合空间索引文件总大小，单位为字节         |
| FreeIndexSize   | int64     | 集合空间索引文件空闲空间大小，单位为字节   |
| TotalLobSize    | int64     | 集合空间大对象文件总大小，单位为字节       |
| FreeLobSize     | int64     | 集合空间大对象文件空闲空间大小，单位为字节 |
| DataCommitLSN   | int64     | 集合空间数据文件最后提交LSN                  |
| IndexCommitLSN  | int64     | 集合空间索引文件最后提交LSN                  |
| LobCommitLSN    | int64     | 集合空间大对象文件最后提交LSN                |
| DataCommitted   | boolean    | 集合空间数据文件当前是否有效提交             |
| IndexCommitted  | boolean    | 集合空间索引文件当前是否有效提交             |
| LobCommitted    | boolean    | 集合空间大对象文件当前是否有效提交           |
| DirtyPage       | int32       | 集合空间大对象文件在开启缓存下脏页数量       |
| Type            | int32      | 集合空间类型，0 表示普通集合空间，1 表示固定（Capped）集合空间 |

##示例##

查看集合空间快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CS" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:42000",
  "GroupName": "db2",
  "Name": "year2015",
  "UniqueID": 124,
  "ID" : 20,
  "LogicalID" : 20,
  "Collection": [
    {
      "Name": "year2015.month01",
      "UniqueID": 532575944705
    }
  ],
  "PageSize": 65536,
  "LobPageSize": 262144,
  "MaxCapacitySize": 26388279066624,
  "MaxDataCapSize": 8796093022208,
  "MaxIndexCapSize": 8796093022208,
  "MaxLobCapSize": 8796093022208,
  "NumCollections": 1,
  "TotalRecords": 1,
  "TotalSize": 306315264,
  "FreeSize": 267714380,
  "TotalDataSize": 155254784,
  "FreeDataSize": 133627820,
  "TotalIndexSize": 151060480,
  "FreeIndexSize": 134086560,
  "TotalLobSize": 0,
  "FreeLobSize": 0,
  "DataCommitLSN": 2484829780,
  "IndexCommitLSN": 2435883264,
  "LobCommitLSN": 0,
  "DataCommitted": true,
  "IndexCommitted": true,
  "LobCommitted": true,
  "DirtyPage": 0,
  "Type": 0
}
...
```
