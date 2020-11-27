[^_^]:
    数据备份恢复
    作者：陈子川
    时间：20190305
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190523


用户在 SequoiaDB 巨杉数据库的备份恢复操作中，将会使用以下工具：
- SDB Shell：用于执行数据库命令
- [sdbrestore][sdbrestore_tool] 工具：用于将备份文件恢复成原数据库文件

全量备份
----
用户可以通过各个数据库驱动程序调用 [backup()][backup_help] 方法来执行 SequoiaDB 的全量备份功能。

### 对整个数据库集群执行全量备份

1. 启动 SDB Shell，并且连接到协调节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 )
   ```

2. 对整个数据库集群执行全量备份

   ```lang-javascript
   > db.backup( { Name: "backupAll", Description: "backup for all" } )
   ```

### 对指定数据分区执行全量备份

1. 启动 SDB Shell，连接到协调节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 )
   ```

2. 对指定数据分区执行全量备份

   ```lang-javascript
   > db.backup( { Name: "backupAll_group1", Description: "backup group1", GroupName: "group1" } )
   ```

增量备份
----

用户执行增量备份依然采用 [backup()][backup_help] 方法，与全量备份方法唯一区别在于调用 [backup()][backup_help] 方法时，将 EnsureInc 方法设置为 true 。

**操作方法**

1. 启动 SDB Shell，并且连接到协调节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 )
   ```

2. 对指定整个数据库集群执行增量备份：

   ```lang-javascript
   > db.backup( { Name: "backupAll", Description: "increase backup data", EnsureInc: true } )
   ```

查看备份信息
----

### 查看备份信息参数说明

| 参数名    | 说明                                                         |
| --------- | ------------------------------------------------------------ |
| Name      | 备份名称，缺省则查看目录下所有备份信息                       |
| Path      | 查看备份的指定路径，缺省为配置参数“bkuppath”中指定的路径     |
| GroupName | 查看指定组的备份信息，缺省为查看全系统备份信息，当需要查看多个组的备份信息可以指定为数组类型，如：```["datagroup1", "datagroup2"]``` |
| Detail    | 是否显示备份的详细信息，缺省为 false                         |

### 查看整个数据库集群备份信息

1. 启动 SDB Shell，并且连接到协调节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 );
   ```

2. 查看备份信息

   ```lang-javascript
   > db.listBackup()
   ```

   输出结果如下：

   ```lang-json
   {
     "Version": 2,
     "Name": "backupAll",
     "ID": 0,
     "Description": "backup for all",
     "NodeName": "sdbserver:11820",
     "GroupName": "SYSCatalogGroup",
     "EnsureInc": false,
     "BeginLSNOffset": -1,
     "EndLSNOffset": 7300,
     "TransLSNOffset": -1,
     "StartTime": "2019-03-05-11:16:33",
     "LastLSN": -1,
     "LastLSNCode": 0,
     "HasError": false
   }
   {
     "Version": 2,
     "Name": "backupAll",
     "ID": 0,
     "Description": "backup for all",
     "NodeName": "sdbserver:11840",
     "GroupName": "group2",
     "EnsureInc": false,
     "BeginLSNOffset": -1,
     "EndLSNOffset": 392,
     "TransLSNOffset": -1,
     "StartTime": "2019-03-05-11:16:33",
     "LastLSN": -1,
     "LastLSNCode": 0,
     "HasError": false
   }
   {
     "Version": 2,
     "Name": "backupAll",
     "ID": 0,
     "Description": "backup for all",
     "NodeName": "sdbserver:11830",
     "GroupName": "group1",
     "EnsureInc": false,
     "BeginLSNOffset": -1,
     "EndLSNOffset": 32,
     "TransLSNOffset": -1,
     "StartTime": "2019-03-05-11:16:33",
     "LastLSN": -1,
     "LastLSNCode": 0,
     "HasError": false
   }
   ```

### 查看指定目录的备份信息

1. 启动 SDB Shell，连接指定节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11830 )
   ```

2. 查看指定目录的备份信息

   ```lang-bash
   > db.listBackup ( { Path: "/opt/sequoiadb/database/data/11830/bakfile" } )
   ```
   
   输出结果如下：
   
   ```lang-json
   {
     "Version": 2,
     "Name": "backupAll",
     "ID": 0,
     "Description": "backup for all",
     "NodeName": "sdbserver:11830",
     "GroupName": "group1",
     "EnsureInc": false,
     "BeginLSNOffset": -1,
     "EndLSNOffset": 32,
     "TransLSNOffset": -1,
     "StartTime": "2019-03-05-11:16:33",
     "LastLSN": -1,
     "LastLSNCode": 0,
     "HasError": false
   }
   ```

数据恢复
----

用户可以使用备份的数据恢复当前集群中的节点或者恢复到离线数据。  

*  恢复当前集群中的节点：执行数据恢复必须确保该节点对应的数据组已停止运行，数据恢复首先会清空原节点的所有数据和日志，然后从备份的数据中恢复配置、数据和日志。
*  恢复到离线数据：可以将全量备份和增量备份的数据不断合并成一份与节点内数据完全相同格式的离线数据，可以在原节点故障后使用该离线数据实现快速恢复。

###数据恢复工具参数说明

使用 sdbrestore 可以进行数据恢复，以下是 sdbrestore 的常用参数：

| 参数          | 缩写 | 说明 |
| ------------- | ---- | ---- |
| --bkpath      | -p   | 备份源数据所在路径 |
| --increaseid  | -i   | 需要恢复到第几次增量备份，缺省恢复到最后一次 ( -1 ） |
| --beginincreaseid | -b | 需要从第几次备份开始恢复，缺省由系统自动计算 ( -1 ) |
| --bkname      | -n   | 需要恢复的备份名 |
| --action      | -a   | 恢复行为，“restore”表示恢复，“list”表示查看备份信息，缺省为“restore” |
| --diaglevel   | -v   | 恢复工具自身的日志级别，缺省为 WARNING ( 3 ) |
| --skipconf    | -s   | 是否需要忽略恢复配置，为 true 时不会改成配置文件 |
| --isSelf      |      | 是否为恢复本节点数据，缺省为“true”；<br>当取值为“false”时，根据如下参数将数据恢复至指定路径：--dbpath，--confpath，--svcname，--indexpath，--logpath，--diagpath，--bkuppath，--replname，--shardname，--catalogname，--httpname |
| --dbpath      |      | 必须配置，数据文件目录 |
| --confpath    |      | 必须配置，配置文件路径， 当设置 `-s true` 时可缺省 |
| --svcname     |      | 必须配置，本地服务名或端口 |
| --indexpath   |      | 索引文件目录 |
| --logpath     |      | 日志文件目录 |
| --diagpath    |      | 诊断日志文件目录 |
| --auditpath   |      | 审记日志文件目录 |
| --bkuppath    |      | 备份文件目录 |
| --archivepath |      | 日志归档目录 |
| --lobmetapath |      | 大对象元数据文件目录 |
| --lobpath     |      | 大对象数据文件目录 |
| --replname    |      | 复制通讯服务名或端口 |
| --shardname   |      | 分区通讯服务名或端口 |
| --catalogname |      | 编目通讯服务名或端口 |
| --httpname    |      | REST 服务名或端口 |

>   **Note:**
>
> * 用户在使用 sdbrestore 恢复工具前，需要先停止对应引擎节点服务。
> * 如果一个复制组包含多个数据节点，必须停止该组中每个数据节点并进行恢复。如果将备份的数据恢复至非备份数据节点，需要把 --isSelf 设置成 false，同时设置相关的配置参数。
> * 用户使用 sdbrestore 工具执行数据恢复工作，每次只能恢复一个引擎节点。如果用户需要恢复一个数据分区中多个引擎节点，可以利用数据分区中全量同步功能，也可以直接将已经恢复了的数据文件通过 scp 远程拷贝方式将其恢复到其他引擎节点上。


### 数据恢复步骤

1. 启动 SDB Shell，并且连接到协调节点

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 )
   ```

2. 停止需要恢复的数据分区

   ```lang-javascript
   db.stopRG( "group1" )
   ```

3. 使用 sdbrestore 工具，将对应备份文件进行恢复，例如恢复 11830 节点的备份文件，备份文件路径为  `/opt/sequoiadb/database/data/11830/bakfile/`，备份名为"backupAll"

   ```lang-bash
   $ sdbrestore -p /opt/sequoiadb/database/data/11830/bakfile/ -n backupAll
   ```

   提示“Restore succeed!”则执行恢复成功：

   ```lang-text
   Check sequoiadb(11830) is not running...OK
   Begin to init dps logs...
   Begin to restore...
   Begin to restore data file: /opt/sequoiadb/database/data/11830/bakfile/backupAll.1 ...
   Begin to restore su: SYSSTAT.1.data ...
   Begin to restore su: SYSSTAT.1.idx ...
   Begin to wait repl bucket empty...
   *****************************************************
   Restore succeed!
   *****************************************************
   ```


[^_^]:
    本文使用到的所有链接及引用。
[backup_help]:manual/reference/Sequoiadb_command/Sdb/backup.md
[sdbrestore_tool]:manual/Distributed_Engine/Maintainance/Backup_Recovery/regular_bar.md#数据恢复
