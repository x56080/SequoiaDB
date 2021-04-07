[^_^]:
    集群模式部署
    作者：秦志强
    时间：
    评审意见
    王涛：
    许建辉：时间：
    市场部：时间：

本文档主要介绍如何在本地主机采用三副本机制部署 SequoiaDB 巨杉数据库集群。集群模式是 SequoiaDB 巨杉数据库部署的标准模式，具有高可用、容灾、数据分区等能力。

集群模式说明
----

在集群环境下，SequoiaDB 巨杉数据库需要三种角色的节点，分别为：[数据节点][data_node]、[编目节点][catalog_node]和[协调节点][coord_node]。集群模式的最小配置中，每种角色的节点都至少启动一个，才能构成完整的集群模式。

集群模式中客户端或应用程序只需连接协调节点，协调节点会对接收到的请求进行解析，并将请求发送到数据节点进行处理。一个或多个节点组成复制组，复制组间的数据无共享。复制组内各节点采用异步数据复制，以保证数据的最终一致性。

检查服务状态
----

在进行集群模式部署前，用户需要在每台数据库服务器上检查 SequoiaDB 配置服务的状态。

```lang-bash
# service sdbcm status
```

系统提示“sdbcm is running”表示服务正在运行，否则执行如下指令重新配置服务程序：

```lang-bash
# service sdbcm start
```


部署
----

部署集群模式主要分为以下步骤：

1. 创建临时协调节点
2. 创建编目节点组和节点
3. 创建数据节点组和节点
4. 创建协调节点组和节点
5. 删除临时协调节点

下述操作步骤假设 SequoiaDB 巨杉数据库程序安装在 `/opt/sequoiadb` 目录下。
> **Note：**
>
> + SequoiaDB 巨杉数据库服务进程全部以 sdbadmin 用户运行，需确保数据库目录都赋予 sdbadmin 读写权限
> + 以下操作只须选择任意一台数据库服务器执行即可

- **创建临时协调节点**

   1. 切换到 sdbadmin 用户

     ```lang-bash
     $ su - sdbadmin
     ```

   2. 启动 SDB Shell 控制台

     ```lang-bash
     $ /opt/sequoiadb/bin/sdb
     ```

   3. 连接本地集群管理服务进程 sdbcm

     ```lang-bash
     > var oma = new Oma("localhost", 11790)
     ```

   4. 创建临时协调节点

     ```lang-bash
     > oma.createCoord(18800, "/opt/sequoiadb/database/coord/18800")
     ```

     > **Note:**
     > - 18800：协调节点服务端口
     > - `/opt/sequoiadb/database/coord/18800`：协调节点的数据文件存放路径

   5. 启动临时协调节点

     ```lang-bash
     > oma.startNode(18800)
     ```

- **创建编目节点组和节点**

   1. 连接临时协调节点
 
     ```lang-bash
     > var db = new Sdb("localhost",18800)
     ```

   2. 创建编目节点组

     ```lang-bash
    >  db.createCataRG("sdbserver1", 11800, "/opt/sequoiadb/database/cata/11800")
    ```

     > **Note:**
     >
     > sdbserver1：服务器主机名

   3. 创建编目节点

     ```lang-bash
     > var cataRG = db.getCataRG()
     > var node1 = cataRG.createNode("sdbserver2", 11800,"/opt/sequoiadb/database/cata/11800")
     > var node2 = cataRG.createNode("sdbserver3", 11800,"/opt/sequoiadb/database/cata/11800")
     ```

   4. 启动编目节点

     ```lang-bash
     > node1.start()
     > node2.start()
     ```

- **创建数据节点组和节点**

   1. 创建数据节点组
 
     ```lang-bash
     > var dataRG = db.createRG("datagroup")
     ```

   2. 创建数据节点

     ```lang-bash
     > dataRG.createNode("sdbserver1", 11820, "/opt/sequoiadb/database/data/11820")
     > dataRG.createNode("sdbserver2", 11820, "/opt/sequoiadb/database/data/11820")
     > dataRG.createNode("sdbserver3", 11820, "/opt/sequoiadb/database/data/11820")
     ```

   3. 启动数据节点组

     ```lang-bash
     > dataRG.start()
     ```

- **创建协调节点组和节点**

   1. 创建协调节点组

     ```lang-bash
     > var coordRG = db.createCoordRG()
     ```

   2. 创建协调节点

     ```lang-bash
     > coordRG.createNode("sdbserver1", 11810, "/opt/sequoiadb/database/coord/11810")
     > coordRG.createNode("sdbserver2", 11810, "/opt/sequoiadb/database/coord/11810")
     > coordRG.createNode("sdbserver3", 11810, "/opt/sequoiadb/database/coord/11810")
     ```

   3. 启动协调节点组：

     ```lang-bash
     > coordRG.start()
     ```

- **删除临时协调节点**

   1. 连接本地集群管理服务进程 sdbcm

     ```lang-bash
     > var oma = new Oma("localhost", 11790)
     ```

   2. 删除临时协调节点

     ```lang-bash
     > oma.removeCoord(18800)
     ```

至此，SequoiaDB 集群模式部署完毕，用户可通过 SDB Shell 进行数据操作。


[^_^]:
     本文使用的所有引用及链接
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[catalog_node]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md


