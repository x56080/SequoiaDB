使用备份的数据恢复当前集群中的节点 或者 恢复到离线数据。  

*  恢复当前集群中的节点：执行数据恢复必须确保该节点对应的数据组已停止运行，数据恢复首先会清空原节点的所有数据和日志，然后从备份的数据中恢复配置、数据和日志。
*  恢复到离线数据：可以将全量备份和增量备份的数据不断合并成一份与节点内数据完全相同格式的离线数据，可以在原节点故障后使用该离线数据实现快速恢复。

##数据恢复工具参数说明##

使用 sdbrestore 可以进行数据恢复，以下是 sdbrestore 的常用参数：

| 参数          | 缩写 | 说明 |
| ------------- | ---- | ---- |
| --bkpath      | -p   | 备份源数据所在路径。 |
| --increaseid  | -i   | 需要恢复到第几次增量备份，缺省恢复到最后一次 ( -1 ）。 |
| --beginincreaseid | -b | 需要从第几次备份开始恢复，缺省由系统自动计算 ( -1 )。 |
| --bkname      | -n   | 需要恢复的备份名。 |
| --action      | -a   | 恢复行为，“restore”表示恢复，“list”表示查看备份信息，缺省为“restore”。 |
| --diaglevel   | -v   | 恢复工具自身的日志级别，缺省为 WARNING ( 3 ) |
| --skipconf    | -s   | 是否需要忽略恢复配置，为 true 时不会改成配置文件 |
| --isSelf      |      | 是否为恢复本节点数据，缺省为“true”；<br>当取值为“false”时，根据如下参数将数据恢复至指定路径：--dbpath，--confpath，--svcname，--indexpath，--logpath，--diagpath，--bkuppath，--replname，--shardname，--catalogname，--httpname。 |
| --dbpath      |      | 必须配置，数据文件目录。 |
| --confpath    |      | 必须配置，配置文件路径， 当 `-s true` 可缺省。 |
| --svcname     |      | 必须配置，本地服务名或端口。 |
| --indexpath   |      | 索引文件目录。 |
| --logpath     |      | 日志文件目录。 |
| --diagpath    |      | 诊断日志文件目录。 |
| --auditpath   |      | 审记日志文件目录。 |
| --bkuppath    |      | 备份文件目录。 |
| --archivepath |      | 日志归档目录。 |
| --lobmetapath |      | 大对象元数据文件目录。 |
| --lobpath     |      | 大对象数据文件目录。 |
| --replname    |      | 复制通讯服务名或端口。 |
| --shardname   |      | 分区通讯服务名或端口。 |
| --catalogname |      | 编目通讯服务名或端口。 |
| --httpname    |      | REST 服务名或端口。 |

>   **Note:**
>
>   如果一个分区组包含多个数据节点，必须停止该组中每个数据节点并进行恢复。如果将备份的数据恢复至非备份数据节点，需要把 --isSelf 设置成 false，同时设置相关的配置参数。

##恢复当前集群中节点的数据##

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ````

2.  得到分区组

    ```lang-javascript
    > dataRG = db.getRG( "group1" )
    ```

3.  停止分区组

    ```lang-javascript
    > dataRG.stop()
    ```

4.  通过终端登入相应分区组的数据节点，执行数据恢复。

    ```lang-javascript
    sdbadmin@hostname1:/opt/sequoiadb> bin/sdbrestore -p database/11820/bakfile -n test_bk
    Check sequoiadb(11820) is not running...OK
    Begin to clean dps logs...
    Begin to clean dms storages...
    Begin to init dps logs...
    Begin to restore...
    Begin to restore data file: 11820/bakfile/test_bk.1 ...
    Begin to restore su: test.1.data ...
    Begin to restore su: test.1.idx ...
    Begin to restore dps logs...
    Begin to wait repl bucket empty...
    *****************************************************
    Restore succeed!
    *****************************************************
    ```

5.  到数据节点目录检查文件是否恢复。

    ```lang-javascript
    sdbadmin@hostname1:/ opt/sequoiadb /database/11820> ls -l
    total 299156
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 16:06 bakfile
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 15:48 diaglog
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 17:39 replicalog
    -rw-r----- 1 sdbadmin sdbadmin 155254784 Nov 13 17:39 test.1.data
    -rw-r----- 1 sdbadmin sdbadmin 151060480 Nov 13 17:39 test.1.idx
    ```

6.  删除该分区组中其它数据节点的所有数据（或者将该节点的所有 .data 和 .idx 文件拷贝至其它数据节点的数据目录和索引目录下，以及将该节点 replicalog 所有日志拷贝至其它数据节点的日志目录下，或者将备份文件拷贝至其它数据节点，并通过 sdbrestore 工具恢复）；重新启动系统。

##恢复到离线数据##

1.  准备用于恢复离线数据的目录

    假定目录为： /backupdata/11820

2.  通过终端登入相应的节点，执行数据恢复。

    ```lang-javascript
    sdbadmin@hostname1:/opt/sequoiadb> bin/sdbrestore -p database/11820/bakfile -n test_bk -s true --isSelf false --dbpath /backupdata/11820 --svcname 11820
    Check sequoiadb(11820) is not running...OK
    Begin to clean dps logs...
    Begin to clean dms storages...
    Begin to init dps logs...
    Begin to restore...
    Begin to restore data file: 11820/bakfile/test_bk.1 ...
    Begin to restore su: test.1.data ...
    Begin to restore su: test.1.idx ...
    Begin to restore dps logs...
    Begin to wait repl bucket empty...
    *****************************************************
    Restore succeed!
    *****************************************************
    ```

5.  到目录检查文件是否恢复。

    ```lang-javascript
    sdbadmin@hostname1:/backupdata/node1> ls -l
    total 299156
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 16:06 bakfile
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 15:48 diaglog
    drwxr-xr-x 2 sdbadmin sdbadmin      4096 Nov 13 17:39 replicalog
    -rw-r----- 1 sdbadmin sdbadmin 155254784 Nov 13 17:39 test.1.data
    -rw-r----- 1 sdbadmin sdbadmin 151060480 Nov 13 17:39 test.1.idx
    ```

6.  可以通过周期性备份 + 周期性恢复，能够实现节点数据的离线同步，在节点故障时快速进行恢复。
