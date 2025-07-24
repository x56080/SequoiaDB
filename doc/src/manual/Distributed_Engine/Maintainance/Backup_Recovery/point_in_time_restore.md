[^_^]:
    时间点恢复

备份和恢复功能是数据库可靠性的重要保证。基于时间点的恢复能力提供了更加灵活快捷的恢复方式，使用户能够在发现故障或错误操作后，根据业务需求尽快地恢复至过去某一指定的一致性时间点，以减小损失。在一致性时间点内，集群中的所有复制组保证全局事务完全一致。用户执行时间点恢复时，在指定一致性时间点后提交的事务将被回滚。用户也可以使用某个数据备份，重新创建一套包含特定数据的环境，用于开发测试。

集群在使用 sdbrestore 恢复后重启，将自动开启恢复模式，以进行时间点恢复。开启恢复模式的集群将会阻止数据访问，特殊情况下如果不需要进行全局一致性恢复，用户可通过 [restoreAbort()][abort] 中止恢复模式。此时数据库将可以提供对外服务，但不保证各数据组中数据的全局一致性。

时间点恢复功能可进行离线恢复和在线恢复：

- 离线恢复：集群在数据库使用 sdbrestore 恢复后重启，如果要保证所有复制组的恢复进程完全同步，可以通过时间点恢复功能，使集群中所有的复制组达到指定的一致点并完成同步。
- 在线恢复：将正在运行的集群恢复至指定的一致性时间点，使用户能够快速地还原数据库到过去某个时间的状态，回滚不需要的修改。用户指定的一致性时间点需存在于当前使用的同步日志中，且该日志还有足够的空间可以写入，否则恢复将无法进行。

>**Note:**
>
> 使用时间点恢复的集群必须支持全局一致性，且所有的备份操作须在全局一致性属性为开启的状态下执行。而集群的全局一致性，需要同时将所有节点的配置参数 [globtranson][configuration] 和 [mvccon][configuration] 设置为 true。

##使用##

###离线恢复###

1. 对所有编目复制组和数据复制组进行全量备份

    ```lang-javascript
    > db.backup({Name:'backupAll'})
    ```

2. 在数据库对外提供服务的同时，定期执行同名增量备份

    ```lang-javascript
    > db.backup({Name:'backupAll', EnsureInc: true})
    ```

3. 停止集群

    ```lang-bash
    $ sdbstop
    ```

4. 恢复所有的编目复制组和数据复制组，如下以编目节点 11820 和数据节点 11830 为例：

    ```lang-bash
    $ sdbrestore -b -1 -p /opt/sequoiadb/database/catalog/11820/bakfile/ -n backupAll
    $ sdbrestore -b -1 -p /opt/sequoiadb/database/data/11830/bakfile/ -n backupAll
    ```

5. 重启集群


    ```lang-bash
    $ sdbstart
    ```

6. 检查是否存在一致性时间点

    ```lang-javascript
    > db.restoreCheck({Time: 0})
    ```

    > **Note:**
    >
    > [restoreCheck()][check] 用于检查指定时间点是否为一致性时间点，也可以用于获取当前最新的一致性时间点。

7. 恢复至最新的一致性时间点

    ```lang-javascript
    > db.restoreToTime({Time: 0})
    ```

    > **Note:**
    >
    > [restoreToTime()][totime] 用于将集群恢复至指定的一致性时间点。


###在线恢复###


1. 无需关闭数据库，直接启用恢复模式

    ```lang-javascript
    > db.restorePrepare()
    ```

    > **Note:**
    >
    > [restorePrepare()][prepare] 用于开启集群的恢复模式，以进行时间点恢复。

2. 检查指定时间点是否为可恢复的一致性时间点

    ```lang-javascript
    > db.restoreCheck({Time: Timestamp("2021-01-01T00:00:00+08:00")});
    ```

3. 还原至指定时间点

    ```lang-javascript
    > db.restoreToTime({Time: Timestamp("2021-01-01T00:00:00+08:00")})
    ```

##注意事项##

SequoiaDB 巨杉数据库中，部分操作会修改数据组中的数据，但属于非事务性的操作，因此在时间点恢复中被认为是不可逆操作。时间点恢复功能的实现基于全局一致性时间，SequoiaDB 使用[时间序列协议][stp]为分布式事务分配全局时间。由于非事务操作没有全局时间，不能按照时间点进行回滚，并且会切断有效的一致性时间窗口，导致无法恢复至非事务操作前的一致性时间点。因此，建议用户避免在事务操作中混用非事务性操作，并在执行非事务性操作之前对数据进行备份。

SequoiaDB 包含的非事务性操作如下：

* [Sdb::analyze()][analyze]
* [Sdb::dropCS()][dropCS]
* [Sdb::renameCS()][renameCS]
* [SdbCS::createCL()][createCL]
* [SdbCS::dropCL()][dropCL]
* [SdbCS::renameCL()][renameCL]
* [SdbCollection::alter()][alter]
* [SdbCollection::deleteLob()][deleteLob]
* [SdbCollection::dropIdIndex()][dropIdIndex]
* [SdbCollection::dropIndex()][dropIndex]
* [SdbCollection::putLob()][putLob]
* [SdbCollection::split()][split]
* [SdbCollection::splitAsync()][splitAsync]
* [SdbCollection::truncate()][truncate]
* [SdbCollection::truncateLob()][truncateLob]




[^_^]:
    本文使用的所有引用及链接
[abort]:manual/Manual/Sequoiadb_Command/Sdb/restoreAbort.md
[check]:manual/Manual/Sequoiadb_Command/Sdb/restoreCheck.md
[totime]:manual/Manual/Sequoiadb_Command/Sdb/restoreToTime.md
[prepare]:manual/Manual/Sequoiadb_Command/Sdb/restorePrepare.md
[stp]:manual/Distributed_Engine/Architecture/Stp/Readme.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[dropCS]:manual/Manual/Sequoiadb_Command/Sdb/dropCS.md
[renameCS]:manual/Manual/Sequoiadb_Command/Sdb/renameCS.md
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[dropCL]:manual/Manual/Sequoiadb_Command/SdbCS/dropCL.md
[renameCL]:manual/Manual/Sequoiadb_Command/SdbCS/renameCL.md
[alter]:manual/Manual/Sequoiadb_Command/SdbCollection/renameCL.md
[deleteLob]:manual/Manual/Sequoiadb_Command/SdbCollection/deleteLob.md
[dropIdIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIdIndex.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[putLob]:manual/Manual/Sequoiadb_Command/SdbCollection/putLob.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
[splitAsync]:manual/Manual/Sequoiadb_Command/SdbCollection/splitAsync.md
[truncate]:manual/Manual/Sequoiadb_Command/SdbCollection/truncate.md
[truncateLob]:manual/Manual/Sequoiadb_Command/SdbCollection/truncateLob.md