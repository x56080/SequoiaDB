[同步日志][replicalog]文件的大小和数量可通过 logfilesz 和 logfilenum 进行设置。logfilesz 和 logfilenum 的默认值分别为 64 和 20。下面以节点"sdbserver1:11820"为例，介绍修改 logfilesz 和 logfilenum 的详细步骤：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 删除全部日志文件

    ```lang-bash
    $ rm -rf /opt/sequoiadb/database/data/11820/replicalog
    ```

3. 修改节点 11820 的配置文件

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

4. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

5. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"logfilesz": "", "logfilenum": ""})
    ```


[^_^]:
    本文使用的所有引用及链接
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#同步日志
