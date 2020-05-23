##概念##

编目节点是一种逻辑节点，用于保存数据库的元数据信息，而不保存用户数据。

编目节点属于编目分区组（可参考 [分区组](infrastructure/replication/overview.md)）。

编目节点中包含以下集合空间：

- **SYSCAT：** 系统编目集合空间，包含以下系统集合：

  | 集合名            |             描述                         |
  |-------------------|------------------------------------------|
  | [SYSCOLLECTIONS](infrastructure/catalog_node/SYSCOLLECTIONS.md) | 保存了该集群中所有的用户集合信息 |
  | [SYSCOLLECTIONSPACES](infrastructure/catalog_node/SYSCOLLECTIONSPACES.md) | 保存了该集群中所有的用户集合空间信息 |
  | [SYSDOMAINS](infrastructure/catalog_node/SYSDOMAINS.md) | 保存了该集群中所有用户域的信息 |
  | [SYSNODES](infrastructure/catalog_node/SYSNODES.md) | 保存了该集群中所有的逻辑节点与复制组信息 |
  | [SYSTASKS](infrastructure/catalog_node/SYSTASKS.md) | 保存了该集群中所有正在运行的后台任务信息 |

- **SYSTEMP：** 系统临时集合空间，可以创建最多4096个临时集合

- **SYSAUTH：** 系统认证集合空间，包含一个用户集合，保存当前系统中所有的用户信息

  |集合名          |    描述                      |
  |----------------|------------------------------|
  | [SYSUSRS](infrastructure/catalog_node/SYSUSRS.md) | 保存了该集群中所有的用户信息 |

- **SYSPROCEDURES：** 系统存储过程集合空间，包含一个集合，用于存储所有的存储过程函数信息

  |    集合名      |   描述                   |
  |----------------|--------------------------|
  | [STOREPROCEDURES](infrastructure/catalog_node/STOREPROCEDURES.md) | 保存所有存储过程函数信息 |

除编目节点外，集群中所有其他的节点不在磁盘中保存任何全局元数据信息。当需要访问其他节点上的数据时，除编目节点外的其他节点需要从本地缓存中寻找集合信息，如果不存在则需要从编目节点获取。

编目节点与其它节点之间主要使用编目服务端口（catalogname 参数）进行通讯。

##管理编目节点##

###新建编目分区组###

>**Note:**
>如果新增节点涉及到新增主机，则需要先按照[集群中新增主机](installation/create_host.md)一节完成主机的主机名和参数配置。

一个数据库集群必须且仅有一个编目分区组，所以新建分区组往往在安装时就已经完成，不需要在安装后执行新建分区组操作。实例见安装指南[集群模式的配置与启动](installation/deployment/command_installation/cluster.md)一节。

- 操作方法：

  ```lang-javascript
  > db.createCataRG( <host>, <service>, <dbpath>, [config] )
  ```

 该命令用于创建编目分区组，同时创建并启动一个编目节点，其中：

  - **host** ：指定编目节点的主机名；

  - **service** ：指定编目节点的服务端口，需要确保该端口号及往后延续的五个端口号未被占用；如端口号设置为11800，应确保11800/11801/11802/11803/11804/11805端口都未被占用；

  - **dbpath** ：数据文件路径，用于存放编目数据文件，需要确保数据管理员（安装时创建，默认为sdbadmin）用户有写权限。如果配置路径不以“/”开头，则数据文件存放路径是数据库管理员用户(默认为sdbadmin)的主目录(默认为/home/sequoiadb) + 配置的路径；

  - **config** ：该参数为可选参数，用于配置更多细节参数，格式必须为 json 格式，细节参数可参考[数据库配置](database_management/database_configuration/configuration_parameters.md)一节；如需要配置日志大小参数｛logfilesz:64｝。

>   **Note:**
>
>   * 编目节点上的事务选项 **transactionon** 默认为开启（为了保证事务日志，编目节点上的日志文件个数 **logfilenum** 需要设置为大于 5）
>
>   * 创建编目分区组，可参考 [Sdb.createCataRG()](reference/Sequoiadb_command/Sdb/createCataRG.md)

###编目分区组中新增节点###

>**Note:**
>如果新增节点涉及到新增主机，则请首先按照在[集群中新增主机](installation/create_host.md)一节完成主机的主机名和参数配置。

随着整个集群中的物理设备的扩展，可以通过增加编目节点来提高编目服务的可靠性。

- 操作方法：

1. 获取编目分区组：

   ```lang-javascript
   > var cataRG = db.getCatalogRG()
   ```

   >   **Note:**
   >
   >   在 Sdb Shell 中也可以使用 [Sdb.getCatalogRG()](reference/Sequoiadb_command/Sdb/getCatalogRG.md) 获取编目分区组

2. 创建一个新的编目节点：

   ```lang-javascript
   > var node1 = cataRG.createNode( <host>, <service>, <dbpath>, [config] )
   ```

   >   **Note:**
   >
   >   **host**、 **service**、 **dbpath** 及 **config** 的设置可参考 [新建编目分区组](infrastructure/catalog_node/catalog_node.md#新建编目分区组)

3. 启动新增的编目节点：

   ```lang-javascript
   > node1.start()
   ```

###查看编目节点###

在 Sdb Shell 中可以查看协调节点的列表：

```lang-javascript
> db.getCatalogRG().getDetail()
```

##故障恢复##

编目节点故障恢复策略与[数据节点](infrastructure/data_node.md)相同。

