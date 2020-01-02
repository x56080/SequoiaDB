##概念##

数据节点为一种逻辑节点，其中保存用户数据信息。

数据节点中没有专门的编目信息集合，因此第一次访问集合前需要向编目节点请求该集合的元数据信息。

在独立模式中，数据节点为单独的服务提供者，直接与应用程序或客户端进行通讯，并且不需要访问任何编目信息。

在集群模式中，数据节点属于某个数据分区组（请参考 [分区组](infrastructure/replication/overview.md)）。

##管理数据节点##

###新增数据分区组###

如果新增节点涉及到新增主机，则请首先按照在[集群中新增主机](installation/create_host.md)一节完成主机的主机名和参数配置。

一个集群中可以配置多个分区组。通过增加分区组，可以充分利用物理设备进行水平扩展，理论上SequoiaDB 可以做到线性的水平扩展能力。

- 操作方法：

1. 建数据分区组，与编目分区组不同的是，该操作不会创建任何数据节点，其中参数为数据组名：

   ```lang-javascript
   > var dataRG = db.createRG( "datagroup1" )
   ```

   >   **Note:**
   >
   >   更多创建数据分区组的内容，请参考 [Sdb.createRG\(\)](reference/Sequoiadb_command/Sdb/createRG.md)

2. 数据组中新增一个数据节点，可以根据需要多次执行该命令来创建多个数据节点：

   ```lang-javascript
   > dataRG.createNode( "sdbserver1", 11820, "/opt/sequoiadb/database/data/11820" )
   ```

   其中：

   **host**：指定数据节点的主机名；

   **service**：指定数据节点的服务端口，请确保该端口号，以及往后延续的5个端口号未被占用；如设置为11820，请确保11820/11821/11822/11823/11824/11825端口都未被占用；

   **dbpath**：数据文件路径，用于存放数据节点的数据文件，请确保数据管理员（安装时创建，默认为sdbadmin）用户有写权限；

   **config**：该参数为可选参数，用于配置更多细节参数，格式必须为 json 格式，参数参见[数据库配置](database_management/database_configuration/configuration_parameters.md)一节；如需要配置日志大小参数｛logfilesz:64｝。

   >   **Note:**
   >
   >   更多创建节点的内容，请参考 [SdbReplicaGroup.createNode\(\)](reference/Sequoiadb_command/SdbReplicaGroup/createNode.md)

3. 启动数据节点：

   ```lang-javascript
   > dataRG.start()
   ```

###分区组中新增节点###

如果新增节点涉及到新增主机，则请首先按照在[集群中新增主机](installation/create_host.md)一节完成主机的主机名和参数配置。

某些分区组可能在创建时设定的副本数较少，随着物理设备的增加，可能需要增加副本数以提高分区组数据可靠性。

- 操作方法：

1. 取数据分区组，参数 groupname 为数据分区组组名：

   ```lang-javascript
   > var dataRG = db.getRG( <groupname> )
   ```

2. 创建一个新的数据节点：

   ```lang-javascript
   > var node1 = dataRG.createNode( <host>, <service>, <dbpath>, [config] )
   ```

   >   **Note:**
   >
   >   **host**、**service**、**dbpath** 及 **config** 的设置请参考 [新增数据分区组](infrastructure/data_node.md#新增数据分区组)

3. 启动新增的数据节点：

   ```lang-javascript
   > node1.start()
   ```

>   **Note:**
>
>   如何创建数据节点组和部署数据节点可以详细请参考 [集群模式](installation/deployment/command_installation/cluster.md)

###查看数据节点###

在 Sdb Shell 中可以查看某个的数据分区组中数据节点的列表，其中参数 groupname 为数据分区组组名：

```lang-javascript
> db.getRG( <groupname> ).getDetail()
```

##故障恢复##

数据节点发生故障后，重新启动时会自动检测数据库目录下 .SEQUOIADB_STARTUP 隐藏文件。

如果该文件存在则说明上次的执行意外终止（例如 kill -9）。对于意外终止的节点，系统会将该数据节点置入崩溃恢复状态。

在崩溃恢复的过程中，数据节点会与该组中的一个正常节点进行全量同步。在这种情况下，被恢复的节点中所有数据作废，同步到的新数据作为基准。请参考 [全量同步](infrastructure/replication/replicate.md#全量同步)

假设该节点没有被意外终止（例如kill -15），则进入增量同步状态。在这种情况下，如果当前其它数据节点中包含的最老日志已经比被恢复节点新，则进入全量同步状态，否则只同步增量日志。请参考 [数据复制](infrastructure/replication/replicate.md)

如果该数据组中所有节点都被意外终止，则需要以独立模式启动一个节点进行本地恢复。在该模式中，数据会被导出并再次导入，以过滤掉所有可能出现的数据损坏。当其中一个节点被本地恢复后，需要将其数据目录拷贝入其它所有数据节点。

>  **Note:**
>
>  可以通过数据的 [备份](database_management/backup_and_recovery/data_backup.md) 与 [恢复](database_management/backup_and_recovery/data_recovery.md) 降低数据丢失的风险。
