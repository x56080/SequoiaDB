##描述##

集合空间快照 $SNAPSHOT_CS 列出当前数据库节点中所有的集合空间，每个集合空间为一条记录。

##标示##

$SNAPSHOT_CS

##字段信息##

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| NodeName        | 字符串     | 集合空间所属节点名（主机名：端口号）         |
| GroupName       | 字符串     | 集合空间所属分区组名                         |
| Name            | 字符串     | 集合空间名                                   |
| UniqueID        | 整型       | 集合空间的UniqueID，在集群上全局唯一         |
| ID              | 整型       | 集合空间在节点上的物理ID                     |
| LogicalID       | 整型       | 集合空间在节点上的逻辑ID                     |
| Collection.Name | 字符串     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | 长整型 | 集合空间所包含的集合的UniqueID               |
| PageSize        | 整型       | 集合空间数据页大小（单位：字节）             |
| LobPageSize     | 整型       | 集合空间大对象数据页大小（单位：字节）       |
| MaxCapacitySize | 长整型     | 集合空间的最大容量上限（单位：字节）         |
| MaxDataCapSize  | 长整型     | 集合空间数据文件最大容量上限（单位：字节）   |
| MaxIndexCapSize | 长整型     | 集合空间索引文件最大容量上限（单位：字节）   |
| MaxLobCapSize   | 长整型     | 集合空间大对象文件最大容量上限（单位：字节） |
| NumCollections  | 整型       | 集合数量                                     |
| TotalRecords    | 整型       | 集合空间的记录总数                           |
| TotalSize       | 长整型     | 集合空间的总大小（单位：字节）               |
| FreeSize        | 长整型     | 集合空间的空闲大小（单位：字节）             |
| TotalDataSize   | 长整型     | 集合空间数据文件总大小（单位：字节）         |
| FreeDataSize    | 长整型     | 集合空间数据文件空闲空间大小（单位：字节）   |
| TotalIndexSize  | 长整型     | 集合空间索引文件总大小（单位：字节）         |
| FreeIndexSize   | 长整型     | 集合空间索引文件空闲空间大小（单位：字节）   |
| TotalLobSize    | 长整型     | 集合空间大对象文件总大小（单位：字节）       |
| FreeLobSize     | 长整型     | 集合空间大对象文件空闲空间大小（单位：字节） |
| DataCommitLSN   | 长整型     | 集合空间数据文件最后提交LSN                  |
| IndexCommitLSN  | 长整型     | 集合空间索引文件最后提交LSN                  |
| LobCommitLSN    | 长整型     | 集合空间大对象文件最后提交LSN                |
| DataCommitted   | 布尔型     | 集合空间数据文件当前是否有效提交             |
| IndexCommitted  | 布尔型     | 集合空间索引文件当前是否有效提交             |
| LobCommitted    | 布尔型     | 集合空间大对象文件当前是否有效提交           |
| DirtyPage       | 整型       | 集合空间大对象文件在开启缓存下脏页数量       |
| Type            | 整型       | 集合空间类型，0 表示普通集合空间，1 表示固定（Capped）集合空间 |

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CS" )
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
