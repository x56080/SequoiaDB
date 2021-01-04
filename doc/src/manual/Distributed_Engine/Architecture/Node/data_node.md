数据节点为一种逻辑节点，其中保存用户数据信息。

数据节点中没有专门的编目信息集合，因此第一次访问集合前需要向编目节点请求该集合的元数据信息。

在独立模式中，数据节点为单独的服务提供者，直接与应用程序或客户端进行通讯，并且不需要访问任何编目信息。

在集群模式中，数据节点属于某个数据复制组，可参考[复制组][Replication]。

管理数据节点
----

如果新增节点涉及到新增主机，用户需先按照在[集群中新增主机][expand]一节完成主机的主机名和参数配置。

### 新增数据复制组

一个集群中可以配置多个复制组。通过增加复制组，可以充分利用物理设备进行水平扩展。

操作方法：

1. 建数据复制组，与编目复制组不同的是，该操作不会创建任何数据节点，其中参数为数据组名

   ```lang-bash
   > var dataRG = db.createRG( "datagroup1" )
   ```

   >   **Note:**
   >
   >   创建数据复制组可参考 [Sdb.createRG\(\)][createRG]

2. 数据组中新增一个数据节点，可以根据需要多次执行该命令来创建多个数据节点

   ```lang-bash
   > dataRG.createNode( "sdbserver1", 11820, "/opt/sequoiadb/database/data/11820" )
   ```

   其中：

   - **host**：指定数据节点的主机名；

   - **service**：指定数据节点的服务端口；用户需确保该端口号，以及往后延续的五个端口号未被占用。如端口号设置为 11820，应确保 11820/11821/11822/11823/11824/11825 端口都未被占用；

   - **dbpath**：数据文件路径用于存放数据节点的数据文件，需确保数据管理员（安装时创建，默认为 sdbadmin）用户有写权限；

   - **config**：该参数为可选参数，用于配置更多细节参数，格式必须为 json 格式，参数参见[数据库配置][cluster_config]一节；如需要配置日志大小参数｛logfilesz:64｝。

   >   **Note:**
   >
   >   创建节点可参考 [SdbReplicaGroup.createNode\(\)][createNode]

3. 启动数据节点：

   ```lang-bash
   > dataRG.start()
   ```

### 复制组中新增节点 ###

某些复制组可能在创建时设定的副本数较少，随着物理设备的增加，可能需要增加副本数以提高复制组数据可靠性。

操作方法：

1. 取数据复制组，参数 groupname 为数据复制组组名

   ```lang-bash
   > var dataRG = db.getRG( <groupname> )
   ```

2. 创建一个新的数据节点

   ```lang-bash
   > var node1 = dataRG.createNode( <host>, <service>, <dbpath>, [config] )
   ```

3. 启动新增的数据节点

   ```lang-bash
   > node1.start()
   ```

>   **Note:**
>
>  部署数据节点时，用户应现在集群中创建有效的编目节点，可参考[集群模式][cluster_deployment]。

### 查看数据节点 ###

在 SDB Shell 中可以查看某个的数据复制组中数据节点的列表，其中参数 groupname 为数据复制组组名：

```lang-bash
> db.getRG( <groupname> ).getDetail()
```

## 故障恢复 ##

数据节点发生故障后，重新启动时会自动检测数据库目录下 `.SEQUOIADB_STARTUP` 隐藏文件。

如果该文件存在则说明上次的执行意外终止（例如 kill -9）。对于意外终止的节点，系统会将该数据节点置入崩溃恢复状态。

在崩溃恢复的过程中，数据节点会与该组中的一个正常节点进行全量同步。在这种情况下，被恢复的节点中所有数据作废，同步到的新数据作为基准。可参考[全量同步][regular_bar]。

假设该节点没有被意外终止（例如kill -15），则进入增量同步状态。在这种情况下，如果当前其它数据节点中包含的最老日志已经比被恢复节点新，则进入全量同步状态，否则只同步增量日志。可参考[数据复制][architecture]。

如果该数据组中所有节点都被意外终止，则需要以独立模式启动一个节点进行本地恢复。在该模式中，数据会被导出并再次导入，以过滤掉所有可能出现的数据损坏。当其中一个节点被本地恢复后，需要将其数据目录拷贝入其它所有数据节点。


[^_^]:
     本文使用的所有引用和链接
[Replication]:manual/Distributed_Engine/Architecture/Replication/architecture.md
[expand]:manual/Distributed_Engine/Maintainance/Expand/expand.md
[createRG]:manual/Manual/Sequoiadb_Command/Sdb/createRG.md
[createNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md
[cluster_deployment]:manual/Deployment/cluster_deployment.md
[regular_bar]:manual/Manual/Database_Configuration/Special_Configuration_Modify/log_synchronization.md#全量同步
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md
[cluster_config]:manual/Manual/Database_Configuration/configuration_parameters.md