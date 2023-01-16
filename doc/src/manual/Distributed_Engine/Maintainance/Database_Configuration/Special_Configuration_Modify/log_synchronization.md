[同步日志][replicalog]文件的大小和数量可通过 logfilesz 和 logfilenum 进行设置。在多副本环境下，复制组中各节点的同步日志大小和数量需一致。为避免数据丢失，修改前需停止写操作并保证复制组内各节点的 LSN 一致。下面以复制组 group1 为例，介绍修改 logfilesz 和 logfilenum 的详细步骤：

1. 通过快照查看复制组中各节点的信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {GroupName: "group1"}, {NodeName: null, IsPrimary: null, CompleteLSN: null})
    ```

    对比主备节点间字段 CompleteLSN 的值，如果保持一致则说明节点 LSN 一致
    
    ```lang-javascript
    {
      "NodeName": "sdbserver1:11820",
      "IsPrimary": true,
      "CompleteLSN": 8000788
    }
    {
      "NodeName": "sdbserver2:11820",
      "IsPrimary": false,
      "CompleteLSN": 8000788
    }
    {
      "NodeName": "sdbserver3:11820",
      "IsPrimary": false,
      "CompleteLSN": 8000788
    }
    ...
    ```

2. 停止复制组 group1

    ```lang-bash
    > db.getRG("group1").stop()
    ```

3. 依次删除复制组中各节点的同步日志

    ```lang-bash
    $ rm -rf /opt/sequoiadb/database/data/11820/replicalog
    ```

4. 依次修改复制组中各节点的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```
  
    将参数 logfilesz 和 logfilenum 修改为 128 和 30

    ```lang-ini
    ...
    logfilesz=128
    logfilenum=30
    ...
    ```

5. 启动复制组 group1

    ```lang-bash
    > db.getRG("group1").start()
    ```

6. 检查复制组中各节点的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {GroupName: "group1"}, {NodeName: null, logfilesz: null, logfilenum: null})
    ```

[^_^]:
    本文使用的所有引用及链接
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#同步日志