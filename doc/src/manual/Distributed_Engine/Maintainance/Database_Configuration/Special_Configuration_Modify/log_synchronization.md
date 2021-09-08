##日志文件##

SequoiaDB 巨杉数据库采用日志方式进行副本间的数据同步。日志文件存在于 `replicalog` 目录中，默认的日志文件总大小为 64MB*20。单个日志文件大小可通过参数 logfilesz 设置，日志文件个数可以通过参数 logfilenum 设置，参数说明可参考[配置项参数][configuration]。当所有日志文件被写满时，日志将重新从第一个文件开始覆盖写入，称为日志翻转。

##同步##

数据组内所有备节点会定期将其他数据节点日志打包下载到本地进行日志回放。同步源并不限于主节点。因为我们期望所有节点的数据版本差距在一个很小的窗口内。当处于这个窗口内时，所有备节点向主节点同步数据。但是当某些节点的数据版本与主节点相差过大时，则选择其他备节点进行同步。当发生版本冲突时，以当前主节点数据版本为准。如果冲突不能解决则进入全量同步。当组内不存在主节点时，任何同步操作均无法进行。

##全量同步##

触发全量同步的原因包括：

- 宕机重启；
- 节点数据版本与其他节点相差过大；
- 数据不一致并且无法修复。

> **Note:**
>
> 正常重启后，如果数据版本仍在可同步范围内则不会触发全量同步。

发生全量同步的节点，会尝试从本地进行全量或阶段性的数据及日志恢复。当本地数据不完整时，缺失的数据将从同组的主节点中提取，并复制到本地；期间同步源发生的数据改变同样会被复制到本地。正在进行全量同步的节点对外不提供服务。

全量同步会极大地影响整个组的性能，甚至导致其他备节点同步性能降低。建议通过如下方式避免全量同步：

- 增加分区，使数据更离散，减少每个复制组的数据量，缩短同步操作的耗时，同时更好地保证数据完整性；
- 增加日志容量，防止日志翻转。

##配置同步日志参数##

参数 logfilesz 和 logfilenum 生效后无法修改。如果要修改必须离线删除全部日志文件，重新配置参数并启动 SequoiaDB，但此举通常会引起全量同步。

1. 关闭要修改配置的节点 11820

    ```lang-javascript
    $ sdbstop -p 11820
    ```

2. 进入该节点目录，删除 replicalog 目录

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    $ rm -rf replicalog/
    ```

3. 进入该节点的配置文件所在位置，修改配置文件

    ```lang-bash
    $ cd /opt/sequoiadb/conf/local/11820
    $ vim sdb.conf
    ```
  
    添加如下内容，将 logfilesz 和 logfilenum 分别配置为 100 和 30：

    ```lang-ini
    ...
    logfilesz=100
    logfilenum=30
    ...
    ```

4. 重新启动节点

    ```lang-javascript
    $ sdbstart -p 11820
    ```

5. 连接协调节点 11810，使用快照查看节点 11820 的配置参数

    ```lang-javascript
    > var db=new Sdb("localhost",11810)
    > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"logfilesz":"","logfilenum":""})
    {
      "logfilesz": 100,
      "logfilenum": 30
    }
    ```


[^_^]:
    本文使用的所有引用及链接
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md