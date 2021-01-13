数据节点是一种逻辑节点，用于保存用户数据信息。数据节点可以在独立模式和集群模式中部署。

- 在[独立模式](installation/deployment/command_installation/standalone.md)中，数据节点为单独的服务提供者，直接与应用程序或客户端进行通讯。

- 在[集群模式](installation/deployment/command_installation/cluster.md)中，数据节点属于某个数据[分区组](infrastructure/replication/overview.md)。

管理数据节点
----

如果新增节点涉及到新增主机，用户需先按照在[集群中新增主机](installation/create_host.md)一节完成主机的主机名和参数配置。

>   **Note:**
>
>  新建数据节点时，用户应先在集群中创建有效的编目节点，可参考[集群模式](installation/deployment/command_installation/cluster.md)。

### 新增数据分区组

一个集群中可以配置多个分区组。通过增加分区组，可以充分利用物理设备进行水平扩展。

操作方法：

1. 创建数据分区组 datagroup1

   ```lang-javascript
   > var dataRG = db.createRG("datagroup1")
   ```

   >   **Note:**
   >
   >   [Sdb.createRG\(\)](reference/Sequoiadb_command/Sdb/createRG.md) 用于创建数据分区组，与编目分区组不同的是，该操作不会创建任何数据节点。

2. 在该数据组中新增一个数据节点

   ```lang-javascript
   > dataRG.createNode("sdbserver1",11820,"/opt/sequoiadb/database/data/11820")
   ```

   [SdbReplicaGroup.createNode\(\)](reference/Sequoiadb_command/SdbReplicaGroup/createNode.md) 用于创建节点，其中

   - **host**：指定数据节点的主机名；

   - **service**：指定数据节点的服务端口；用户需确保该端口号，以及往后延续的五个端口号未被占用。如端口号设置为 11820，应确保 11820/11821/11822/11823/11824/11825 端口都未被占用；

   - **dbpath**：数据文件路径用于存放数据节点的数据文件，需确保数据管理员（安装时创建，默认为 sdbadmin）用户有写权限；

   - **config**：该参数为可选参数，用于配置更多细节参数，格式必须为 json 格式，参数参见[数据库配置](database_management/database_configuration/configuration_parameters.md)一节；如需要配置日志大小参数｛logfilesz:64｝。

   >   **Note:**
   >
   >   用户可以根据需要多次执行该命令在分区组中创建多个数据节点，每个分区组最多可创建七个数据节点。

3. 启动数据节点

   ```lang-javascript
   > dataRG.start()
   ```

### 分区组中新增节点 ###

如果分区组在创建时设定的副本数较少，随着物理设备的增加，可能需要增加副本数以提高分区组数据的可靠性。

操作方法：

1. 获取数据分区组 datagroup1

   ```lang-javascript
   > var dataRG = db.getRG("datagroup1")
   ```

2. 创建一个新的数据节点

   ```lang-javascript
   > var node1 = dataRG.createNode("sdbserver1",11830,"/opt/sequoiadb/database/data/11830")
   ```

3. 启动新增的数据节点

   ```lang-javascript
   > node1.start()
   ```

### 查看数据节点 ###

在 SDB Shell 中查看数据分区组 datagroup1 中数据节点的列表

```lang-javascript
> db.getRG("datagroup1").getDetail()
```

## 故障恢复 ##

对于意外终止的节点：

1. 数据节点发生故障后，重新启动时会自动检测数据库目录下 `.SEQUOIADB_STARTUP` 隐藏文件；
2. 如果该文件存在则说明上次的执行意外终止（例如 `kill -9`），对于意外终止的节点，系统会将该数据节点置入崩溃恢复状态；
3. 该节点在崩溃恢复的过程中，会与同组的一个正常节点进行[全量同步](infrastructure/replication/replicate.md#全量同步)。

对于没有被意外终止的节点：

1. 数据节点发生故障后，重新启动时会自动检测数据库目录下 `.SEQUOIADB_STARTUP` 隐藏文件；
2. 如果该文件不存在则说明该节点没有被意外终止（例如 `kill -15`），则进入增量同步状态，可参考[数据复制](infrastructure/replication/replicate.md)；
3. 在增量同步的情况下，若当前其它数据节点上的日志已经发生了覆写，导致被恢复节点还未获取到的复制日志被覆盖，则进入全量同步状态。

对于所有节点都被意外终止：

1. 数据节点发生故障后，所有节点都会被自动拉起，并在数据组内选取主节点；
2. 所选取的主节点将进行数据重建，重建过程中，主节点会把数据导出至磁盘，再从磁盘导入；
3. 主节点完成数据重建后，所有备节点均进入全量同步状态。

