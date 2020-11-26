[^_^]:
    复制组原理
    作者：余婷
    时间：20190314
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190814

复制组副本间通过拷贝和重放事务日志来实现数据同步。

复制组成员
----

一个复制组由一个或者多个节点组成。复制组内有两种不同的角色：主节点和备节点。正常情况下，一个复制组内有且只有一个主节点，其余为备节点。

### 主节点 ###

主节点是复制组内唯一接收写操作的成员。当发生写操作时，主节点写入数据，并记录事务日志 replicalog。备节点从主节点异步复制 replicalog，并通过重放 replicalog 来复制数据。  

![主节点][primary]

### 备节点 ###

- 备节点持有主节点数据的副本，一个复制组可以有多个备节点。

- 备节点从主节点异步复制 replicalog，并重放 replicalog 来复制数据。复制数据的过程需要一定的时间，有可能经过一段时间才能从备节点上访问到更新后的数据，SequoiaDB 的复制组默认是保证最终一致性。

![复制组示意图][replication]

事务日志replicalog
----

节点之间，通过事务日志进行副本间的数据同步，事务日志文件存在于节点数据目录下的 `replicalog` 目录。以 11830 节点的数据目录为例 ，当节点首次启动时，节点进程会生成以上的 replicalog 文件：

```lang-text
$ ls -l /opt/sequoiadb/database/data/11830/replicalog
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:50 sequoiadbLog.0
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.1
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.2
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.3
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.4
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.5
-rwx------ 1 sdbadmin sdbadmin_group 67174400 3月 11 12:49 sequoiadbLog.6
-rwx------ 1 sdbadmin sdbadmin_group    69632 3月 11 12:49 sequoiadbLog.meta
```

> **Note:**
>
> 文件的大小和个数可以通过 [logfilesz][logfilesz] 和 [logfilenum][logfilenum] 参数分别进行设置。默认日志文件大小为 64MB（不包括头大小），日志个数是 20 个。

用户可以通过 [sdbdpsdump][dpsdump] 工具查看写入的事务日志。

1. 插入一条记录

   ```lang-bash
   > db.sample.employee.insert( { a: 1 } ) 
   ```

2. 用工具查看事务日志

   ```lang-bash
   $ ./bin/sdbdpsdump -s ./database/data/11830/replicalog
   ```

   输出结果如下：

   ```lang-text
   ...
    Version: 0x00000001(1)
    LSN    : 0x00000000000000ec(236)
    PreLSN : 0x000000000000009c(156)
    Length : 80
    Type   : INSERT(1)
    FullName : sample.employee
    Insert : { "_id": { "$oid": "5c88afe31a3f5822754040d0" } , "a": 1 }
   ```

   > **Note:**
   > 
   > - LSN 是指该条日志在日志文件中的偏移，每条事务日志都对应唯一的 LSN 号。
   > - 日志是循环写入文件的。当最后一个日志文件写满时，下一条事务日志会从第一个日志文件开始写，第一个文件之前的日志会被覆盖掉。

数据复制
----

数据复制为复制组中节点之间的同步机制。

### 增量同步 ###

在数据节点和编目节点中，任何增删改操作均会写入日志。节点会将日志写入日志缓冲区，然后再异步写入本地磁盘。

数据复制在两个节点间进行：

- 源节点：含有新数据的节点
- 目标节点：请求进行数据复制的节点

目标节点会选择一个与其数据最接近的节点，然后向它发送一个复制请求。源节点收到复制请求后，会打包目标节点请求的同步点之后的日志，并发送给目标节点。目标节点接收到同步数据包后，会重放事务日志中的操作。

节点之前的复制有两种状态：

- 对等状态（Peer）：目标节点请求的日志，存在于源节点的日志缓冲区
- 远程追赶状态（Remote Catchup）：目标节点请求的日志，不存在于源节点的日志缓冲区中，但存在于源节点的日志文件中

如果目标节点请求的日志，已经不存在于源节点的日志文件中，则目标节点进入全量同步状态。

当两节点处于对等状态时，源节点上可以直接从内存中获取日志。因此目标节点选择源节点时，总会尝试选择距离自己当前日志点最近的节点，使请求的日志尽量落在内存中。所以源节点不一定总是主节点。

### 全量同步 ###

在复制组内，有些情况下需要进行数据全量同步，才能保障节点之间数据的一致性。以下情况需要进行全量同步：

- 一个新的节点加入复制组
- 节点故障导致数据损坏
- 节点日志远远落后于其他节点，即当前节点的日志已经不存在于其他节点的日志文件中

全量同步在两个节点间进行：

- 源节点：指含有有效数据的节点，全量同步的源节点必定是主节点
- 目标节点：指请求进行全量同步的节点，全量同步时，该节点下原有的数据会被废弃

![全量同步示意图][full_sync]

全量同步发生时，目标节点会定期向源节点请求数据，源节点将数据打包后作为大数据块发送给目标节点。当目标节点重做该数据块内所有数据后，向源节点请求新的数据块。

读写分离
----

协调节点通过将读请求发送至不同的数据副本，以降低读写 I/O 冲突，提升集群整体的吞吐量。

### 写请求处理 ###

所有的写请求都只会发往主节点。

### 读请求处理 ###

读请求会按照会话的属性选择组内节点。

- 如果该会话上发生过写操作，读请求会选择主节点，即读我所写。
- 如果该会话上未发生过写操作，读请求会随机选择组内的任意一个节点。
- 如果该会话上配置了选择节点的策略 [db.setSessionAttr()][session_attr]，则读请求会优先按照策略处理。

例如，集合 sample.employee 落在数据组 group1 上，group1 上有三个节点 `sdbserver1:11830`、`sdbserver2:11830` 和 `sdbserver3:11830`，其中 `sdbserver1:11830` 是主节点：

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> // 插入数据后，查询走主节点
> db.sample.employee.insert( { a: 1} )                       
> db.sample.employee.find().explain( {Run: true } )
{
  "NodeName": "sdbserver1:11830"
  "GroupName": "group1"
  "Role": "data"
  ...
}
>
> // 设置会话上读请求的策略：优先从备节点上读，查询走备节点
> db.setSessionAttr( { PreferedInstance: 's' } )
> db.sample.employee.find().explain( {Run: true } )
{
  "NodeName": "sdbserver2:11830"
  "GroupName": "group1"
  "Role": "data"
  ...
}
>
> // 再次插入数据后，查询走主节点
> db.sample.employee.insert( { a: 1} )
> db.sample.employee.find().explain( {Run: true } )
{
  "NodeName": "sdbserver1:11830"
  "GroupName": "group1"
  "Role": "data"
  ...
}
```

节点一致性
----

在分布式系统中，一致性是指数据在多个副本之间数据保持一致的特性。

### 最终一致性 ###

为了提升数据的可靠性和实现数据的读写分离，SequoiaDB 巨杉数据库默认采用“最终一致性”策略。在读写分离时，读取的数据在某一段时间内可能不是最新的，但副本间的数据最终是一致的。

### 强一致性 ###

写请求处理成功后，后续读到的数据一定是当前组内最新的，但是这样会降低复制组的写入性能。

用户可以通过 [cs.createCL()][create_cl] 在创建集合时指定 ReplSize 属性，来提高数据的一致性和可靠性。

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> // 写操作需要等待所有的副本都完成才返回，强一致性
> db.sample.createCL( 'employee1', { ReplSize: 0 })
> // 写操作等待一个副本完成就会返回，最终一致性
> db.sample.createCL( 'employee2', { ReplSize: 1 })
```

[^_^]:
    本文使用到的所有链接及引用
[primary]: images/infrastructure/Replication/primary.png
[replication]: images/infrastructure/Replication/replication.png
[dpsdump]: manual/Maintainance/Mgmt_Tools/dpsdump.md
[full_sync]:images/infrastructure/Replication/full_sync.png
[create_cl]:manual/reference/Sequoiadb_command/SdbCS/createCL.md
[session_attr]: manual/reference/Sequoiadb_command/Sdb/setSessionAttr.md
[logfilesz]: manual/database_management/database_configuration/configuration_parameters.md
[logfilenum]: manual/database_management/database_configuration/configuration_parameters.md



