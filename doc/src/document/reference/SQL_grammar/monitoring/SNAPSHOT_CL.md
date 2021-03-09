##描述##

集合快照 $SNAPSHOT_CL 列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标示##

$SNAPSHOT_CL

##字段信息##

| 字段名              | 类型          | 描述                                                    |
| ------------------- | ------------- | ------------------------------------------------------- |
| Name                | 字符串        | 集合完整名                                              |
| UniqueID            | 长整型        | 集合的UniqueID，在集群上全局唯一                        |
| CollectionSpace     | 字符串        | 集合所属集合空间名                                      |
| Details.NodeName            | 字符串        | 集合所属节点名（主机名：端口号）                        |
| Details.GroupName           | 字符串        | 集合所属分区组名                                        |
| Details.InternalV   | 整型          | 集合视图的版本                                          |
| Details.ID          | 整型          | 集合 ID，范围 0 ~ 4095，集合空间内唯一                  |
| Details.LogicalID   | 整型          | 集合逻辑 ID                                             |
| Details.Sequence    | 整型          | 序列号                                                  |
| Details.Indexes     | 整型          | 该集合所包含的索引数量                                  |
| Details.Status      | 字符串        | 集合当前状态<br>- Free：空闲<br>- Normal：正常<br>- Dropped：被删除<br>- Offline Reorg Shadow Copy Phase：离线重组复制阶段<br>- Offline Reorg Truncate Phase：离线重组清除阶段<br>- Offline Reorg Copy Back Phase：离线重组重入阶段<br>- Offline Reorg Rebuild Phase：离线重组重建索引阶段 |
| Details.Attribute   |  字符串       | 属性 |
| Details.CompressionType | 字符串    | 压缩类型 |
| Details.DictionaryCreated | 布尔型  | 是否创建压缩字典  |
| Details.DictionaryVersion   | 整型  | 压缩字典版本 |
| Details.PageSize            | 整型  | 集合页的大小  |
| Details.LobPageSize         | 整型  | 大对象页的大小   |
| Details.TotalRecords        | 长整型        | 集合的记录总数                                          |
| Details.TotalLobs           | 长整型        | 集合的大对象总数  |
| Details.TotalDataPages      | 整型          | 集合的数据页总数                                        |
| Details.TotalIndexPages     | 整型          | 集合的索引页总数                                        |
| Details.TotalLobPages       | 整型          | 集合的大对象页总数                                      |
| Details.TotalDataFreeSpace  | 长整型        | 集合的数据空闲空间（单位：字节）                        |
| Details.TotalIndexFreeSpace | 长整型        | 集合的索引空闲空间（单位：字节）                        |
| Details.CurrentCompressionRatio | 浮点型    | 集合的的压缩率                                          |
| Details.DataCommitLSN   | 长整型     | 集合数据文件最后提交LSN    |
| Details.IndexCommitLSN  | 长整型     | 集合索引文件最后提交LSN    |
| Details.LobCommitLSN    | 长整型     | 集合大对象文件最后提交LSN  |
| Details.DataCommitted   | 布尔型     | 集合数据文件当前是否有效提交 |
| Details.IndexCommitted  | 布尔型     | 集合索引文件当前是否有效提交 |
| Details.LobCommitted    | 布尔型     | 集合大对象文件当前是否有效提交 |
| Details.TotalDataRead   | 长整型 | 集合数据读请求 |
| Details.TotalIndexRead  | 长整型 | 集合索引读请求 |
| Details.TotalDataWrite  | 长整型 | 集合数据写请求 |
| Details.TotalIndexWrite | 长整型 | 集合索引写请求 |
| Details.TotalUpdate     | 长整型 | 集合更新记录数量 |
| Details.TotalDelete     | 长整型 | 集合删除记录数量 |
| Details.TotalInsert     | 长整型 | 集合插入记录数量 |
| Details.TotalSelect     | 长整型 | 集合选择记录数量 |
| Details.TotalRead       | 长整型 | 集合读取记录数量 |
| Details.TotalWrite      | 长整型 | 集合写入记录数量 |
| Details.TotalTbScan     | 长整形 | 集合使用表扫描次数 |
| Details.TotalIxScan     | 长整型 | 集合使用索引扫描次数 |
| Details.ResetTimestamp  | 时间戳 | 重置快照的时间 |

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CL" )
{
  "Name": "newcs.newcl",
  "UniqueID": 4294967297,
  "CollectionSpace": "newcs",
  "Details": [
    {
      "NodeName": "hostname:20000",
      "GroupName": "db1",
      "InternalV": 1,
      "ID": 0,
      "LogicalID": 0,
      "Sequence": 1,
      "Indexes": 2,
      "Status": "Normal",
      "Attribute": "Compressed",
      "CompressionType": "lzw",
      "DictionaryCreated": true,
      "DictionaryVersion": 1,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "TotalRecords": 1052000,
      "TotalLobs": 0,
      "TotalDataPages": 1259,
      "TotalIndexPages": 1130,
      "TotalLobPages": 0,
      "TotalDataFreeSpace": 38324,
      "TotalIndexFreeSpace": 17092920,
      "CurrentCompressionRatio": 0.3,
      "DataCommitLSN": 132007488,
      "IndexCommitLSN": 2184160,
      "LobCommitLSN": 0,
      "DataCommitted": true,
      "IndexCommitted": true,
      "LobCommitted": true,
      "TotalDataRead": 0,
      "TotalIndexRead": 0,
      "TotalDataWrite": 1052000,
      "TotalIndexWrite": 2078000,
      "TotalUpdate": 0,
      "TotalDelete": 0,
      "TotalInsert": 1052000,
      "TotalSelect": 0,
      "TotalRead": 0,
      "TotalWrite": 1052000,
      "TotalTbScan": 0,
      "TotalIxScan": 6,
      "ResetTimestamp": "2019-06-24-17.12.30.279933"
    }
  ]
}
...
```
