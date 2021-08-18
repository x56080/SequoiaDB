
热备节点是一种逻辑节点，存储在热备组中，用于替换故障的数据节点。热备节点在加入其他数据分区组前，不存储任何数据；在加入指定分区组后，将自动同步该分区组的所有数据。

一般来说，热备节点的处理流程如下：

1. 创建热备组
2. 在热备组中创建若干个节点并启动
3. 当数据节点出现故障时，在热备组中通过 [detachNode()](reference/Sequoiadb_command/SdbReplicaGroup/detachNode.md) 方法，得到一个不属于任何组的热备节点
4. 将该热备节点通过 [attachNode()](reference/Sequoiadb_command/SdbReplicaGroup/attachNode.md) 方法，加入故障节点所在分区组
5. 将故障的节点从该分区组中剔除

##新建热备组##

同一集群内只可以存在一个热备组，且统一命名为“SYSSpare”。

```lang-javascript
> db.createSpareRG()
```

>**Note:**
>
> [createSpareRG()](reference/Sequoiadb_command/Sdb/createSpareRG.md) 用于创建热备组，该操作不会创建任何数据节点。

##热备组中新增节点##

热备组内节点间不进行心跳检测，因此该组内不存在主节点。

1. 获取热备组

   ```lang-javascript
   var spareRG = db.getRG("SYSSpare")
   ```

2. 创建一个新的热备节点

   ```lang-javascript
   var node1 = spareRG.createNode("sdbserver",11860,"/opt/sequoiadb/database/data/11860")
   ```

3. 启动新增的热备节点

   ```lang-javascript
   > node1.start()
   ```

##查看热备节点##

在 SDB Shell 中查看热备组中节点的列表

```lang-javascript
db.getRG("SYSSpare").getDetail()
```

##使用##

当分区组中的数据节点发生无法自动修复的故障（如磁盘损坏）时，建议使用热备节点替换该故障节点，保证分区组的可用性。

下述以数据分区组 group1 中 11820 节点故障为例，使用热备节点 11860 进行替换操作。

1. 获取热备组

   ```lang-javascript
   var spareRG = db.getRG("SYSSpare")
   ```

2. 将热备节点从热备组中分离

   ```lang-javascript
   spareRG.detachNode("sdbserver",11860,{KeepData:false})
   ```

   >**Note:**
   >
   > 将热备节点从热备组中分离可参考 [detachNode()](reference/Sequoiadb_command/SdbReplicaGroup/detachNode.md)。

3. 将分离后的 11860 节点加入分区组 group1 中

   ```lang-javascript
   db.getRG("group1").attachNode("sdbserver",11860,{KeepData:false})
   ```

   > **Note:**
   >
   > 将已分离的节点加入其他分区组可参考 [attachNode()](reference/Sequoiadb_command/SdbReplicaGroup/attachNode.md)。

4. 通过[节点健康快照](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md)检查 11860 节点的 CompleteLSN 字段值是否与同组的主节点一致，如果一致则表示数据同步完成

   ```lang-javascript
   > db.snapshot(SDB_SNAP_HEALTH,{},{"NodeName":"","CompleteLSN":""})
   ```

5. 将故障节点 11820 从分区组中剔除

   ```lang-javascript
   > db.getRG("group1").detachNode("sdbserver",11820,{KeepData:false})
   ```
