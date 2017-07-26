##描述##

集合快照 SDB_SNAP_COLLECTIONS 列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标示##

SDB_SNAP_COLLECTIONS

##字段信息##

由于数据节点与编目节点保存的集合信息不同，集合快照在协调节点与其它节点所返回的结构有所不同：

##非协调节点字段信息##

| 字段名              | 类型          | 描述                                                    |
| ------------------- | ------------- | ------------------------------------------------------- |
| Name                | 字符串        | 集合完整名                                              |
| Details.ID          | 整型          | 集合 ID，范围 0 ~ 4095，集合空间内唯一                  |
| Details.LogicalID   | 整型          | 集合逻辑 ID                                             |
| Details.Sequence    | 整型          | 序列号                                                  |
| Details.Indexes     | 整型          | 该集合所包含的索引数量                                  |
| Details.Status      | 字符串        | 集合当前状态<br>- Free：空闲<br>- Normal：正常<br>- Dropped：被删除<br>- Offline Reorg Shadow Copy Phase：离线重组复制阶段<br>- Offline Reorg Truncate Phase：离线重组清除阶段<br>- Offline Reorg Copy Back Phase：离线重组重入阶段<br>- Offline Reorg Rebuild Phase：离线重组重建索引阶段 |
| Details.TotalRecords        | 长整型        | 集合的记录总数                                          |
| Details.TotalDataPages      | 整型          | 集合的数据页总数                                        |
| Details.TotalIndexPages     | 整型          | 集合的索引页总数                                        |
| Details.TotalLobPages       | 整型          | 集合的大对象页总数                                      |
| Details.TotalDataFreeSpace  | 长整型        | 集合的数据空闲空间（单位：字节）                        |
| Details.TotalIndexFreeSpace | 长整型        | 集合的索引空闲空间（单位：字节）                        |
| Details.CurrentCompressionRatio | 浮点型    | 集合的的压缩率                                          |
| DataCommitLSN   | 长整型     | 集合数据文件最后提交LSN    |
| IndexCommitLSN  | 长整型     | 集合索引文件最后提交LSN    |
| LobCommitLSN    | 长整型     | 集合大对象文件最后提交LSN  |
| DataCommitted   | 布尔型     | 集合数据文件当前是否有效提交 |
| IndexCommitted  | 布尔型     | 集合索引文件当前是否有效提交 |
| LobCommitted    | 布尔型     | 集合大对象文件当前是否有效提交 |

##协调节点字段信息##

| 字段名                            | 类型          | 描述                                                    |
| --------------------------------- | ------------- | ------------------------------------------------------- |
| Name                              | 字符串        | 集合完整名                                              |
| Details.GroupName                 | 字符串        | 节点所在分区组名                                        |
| Details.Group.ID                  | 整型          | 集合 ID，范围0\~4096，集合空间内唯一                    |
| Details.Group.LogicalID           | 整型          | 集合逻辑 ID                                             |
| Details.Group.Sequence            | 整型          | 序列号                                                  |
| Details.Group.Indexes             | 整型          | 该集合所包含的索引数量                                  |
| Details.Group.Status              | 字符串        | 集合当前状态<br>- Free：空闲<br>- Normal：正常<br>- Dropped：被删除<br>- Offline Reorg Shadow Copy Phase：离线重组复制阶段<br>- Offline Reorg Truncate Phase：离线重组清除阶段<br>- Offline Reorg Copy Back Phase：离线重组重入阶段<br>- Offline Reorg Rebuild Phase：离线重组重建索引阶段 |
| Details.Group.TotalRecords        | 长整型        | 集合的记录总数                                          |
| Details.Group.TotalDataPages      | 整型          | 集合的数据页总数                                        |
| Details.Group.TotalIndexPages     | 整型          | 集合的索引页总数                                        |
| Details.Group.TotalDataFreeSpace  | 长整型        | 集合的数据空闲空间（单位：字节）                        |
| Details.Group.TotalIndexFreeSpace | 长整型        | 集合的索引空闲空间（单位：字节）                        |
| Details.Group.NodeName            | 字符串        | 节点名（主机名 + 端口）                                 |

##非协调节点示例##

```lang-javascript
> db.snapshot( SDB_SNAP_COLLECTIONS )
{
  "Name": "foo.bar",
  "Details": [
    {
      "ID": 0,
      "LogicalID": 0,
      "Sequence": 1,
      "Indexes": 8,
      "Status": "Normal",
      "TotalRecords": 0,
      "TotalDataPages": 0,
      "TotalIndexPages": 6,
      "TotalLobPages": 0,
      "TotalDataFreeSpace": 0,
      "TotalIndexFreeSpace": 196545,
      "CurrentCompressionRatio": 1,
      "DataCommitLSN": 160,
      "IndexCommitLSN": 180,
      "LobCommitLSN": -1,
      "DataCommitted": false,
      "IndexCommitted": false,
      "LobCommitted": true
    }
  ]
}
```

##协调节点示例##

```lang-javascript
> coord.snapshot( SDB_SNAP_COLLECTIONS )
{
  "Name": "susefoo.susebar",
  "Details": [
    {
      "GroupName": "group1",
      "Group": [
        {
          "ID": 0,
          "LogicalID": 0,
          "Sequence": 1,
          "Indexes": 1,
          "Status": "Normal",
          "TotalRecords": 1,
          "TotalDataPages": 1,
          "TotalIndexPages": 2,
          "TotalLobPages": 0,
          "TotalDataFreeSpace": 4004,
          "TotalIndexFreeSpace": 4046,
          "NodeName": "hostname1:11820"
        }
      ]
    }
  ]
}
```
