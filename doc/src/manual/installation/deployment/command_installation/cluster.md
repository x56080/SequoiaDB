##集群模式的配置与启动##

- 集群模式是启动 SequoiaDB 的标准模式，至少需要三个节点。

- 在集群环境下，SequoiaDB 数据库需要三种角色的节点，分别为：

  - [数据节点](manual/infrastructure/Node/data_node.md)
  - [编目节点](manual/infrastructure/Node/catalog_node.md)
  - [协调节点](manual/infrastructure/Node/coord_node.md)

- 集群模式的最小配置中，每种角色的节点至少启动一个，才能构成完整的集群模式。

- 集群模式中客户端或应用程序直接连接到协调节点，其余数据节点与编目节点对应用程序完全透明。

- 应用程序本身不需关心数据存放在哪个数据节点，协调节点会对接收到的请求解析，自动将其发送到需要的数据节点上进行处理。

- 在集群模式下，复制组之间的数据无共享，复制组内的节点间进行异步数据复制，保证数据的最终一致性。

>**Note:**  
>1. 在配置集群模式时，请先确保服务器与主机名的映射关系正确，详细请参考[Linux系统要求](installation/system/system_requirement.md) ，确保各节点之间能相互通信，将节点的防火墙关闭。  
>2. 参看[Linux推荐配置](installation/system/linux_suggest_settings.md)中关于NUMA的条目，NUMA对SequoiaDB的运行有影响。尤其是高负荷的生产环境，建议关闭NUMA或者使用“numactl --interleave=all”启动数据库服务。

**说明：**

1. 本节以[高可用](installation/deployment/command_installation/planning_database_deployment.md)的方式部署为例，介绍配置和启动步骤。

2. 以下操作步骤假设 SequoiaDB 程序安装在 /opt/sequoiadb 目录下。

3. SequoiaDB 服务进程全部以 sdbadmin 用户运行，请确保所有数据库目录都赋予 sdbadmin 读写权限。

- 步骤一：检查 SequoiaDB 的配置服务状态
  1. 在每台数据库服务器上检查 SequoiaDB 配置服务状态：

     ```lang-bash
     # service sdbcm status
     ```
  2. 确认系统提示“sdbcm is running”表示服务正在运行，否则请执行如下命令重新配置服务程序：

     ```lang-bash
     # service sdbcm start
     ```

- 步骤二：启动一个临时协调节点（该节点只是为了创建其它节点而临时使用，安装完毕后需要删除该节点）
  1. 切换到 sdbadmin 用户

     ```lang-bash
     # su - sdbadmin
     ```
  2. 在任意一台数据库服务器上（以下步骤都只需要在这台服务器上操作），启动 SequoiaDB Shell 控制台

     ```lang-bash
     $ /opt/sequoiadb/bin/sdb
     ```
  3. 连接到本地的集群管理服务进程 sdbcm

     ```lang-javascript
     > var oma = new Oma("localhost", 11790)
     ```
  4. 创建临时协调节点

     ```lang-javascript
     > oma.createCoord(18800, "/opt/sequoiadb/database/coord/18800")
     ```
  5. 启动临时协调节点

     ```lang-javascript
     > oma.startNode(18800)
     ```

- 步骤三：通过命令配置和启动编目节点
  1. 连接到临时协调节点，在 shell 命令中输入：

     ```lang-javascript
     > var db = new Sdb("localhost",18800)
     ```

     其中18800为协调节点端口号
  2. 创建一个编目节点组

     ```lang-javascript
     > db.createCataRG("sdbserver1", 11800, "/opt/sequoiadb/database/cata/11800")
     ```


     sdbserver1：第一台服务器主机名。

     11800：为编目节点服务端口。

     /opt/sequoiadb/database/cata/11800：为编目节点的数据文件存放路径。

  3. 添加另外两个编目节点

     ```lang-javascript
     > var cataRG = db.getRG("SYSCatalogGroup");
     > var node1 = cataRG.createNode("sdbserver2", 11800,"/opt/sequoiadb/database/cata/11800")
     > var node2 = cataRG.createNode("sdbserver3", 11800,"/opt/sequoiadb/database/cata/11800")
     ```
  	>**Note:**  
  	> createNode() 的第一个参数建议使用“主机名”。
  4. 启动编目节点组

     ```lang-javascript
     > node1.start()
     > node2.start()
     ```


- 步骤四：通过命令配置和启动数据节点
  1. 创建数据节点组

     ```lang-javascript
     > var dataRG = db.createRG("datagroup")
     ```
  2. 添加数据节点

     ```lang-javascript
     > dataRG.createNode("sdbserver1", 11820, "/opt/sequoiadb/database/data/11820")
     > dataRG.createNode("sdbserver2", 11820, "/opt/sequoiadb/database/data/11820")
     > dataRG.createNode("sdbserver3", 11820, "/opt/sequoiadb/database/data/11820")
     ```

     >**Note:**  
  		> createNode() 的第一个参数建议使用“主机名”。
  3. 启动数据节点组

     ```lang-javascript
     > dataRG.start()
     ```

- 步骤五：部署启动协调节点
  1. 创建协调节点组

     ```lang-javascript
     > var rg = db.createCoordRG()
     ```
  2. 创建协调节点

     ```lang-javascript
     > rg.createNode("sdbserver1", 11810, "/opt/sequoiadb/database/coord/11810")
     > rg.createNode("sdbserver2", 11810, "/opt/sequoiadb/database/coord/11810")
     > rg.createNode("sdbserver3", 11810, "/opt/sequoiadb/database/coord/11810")
     ```
  3. 启动协调节点

     ```lang-javascript
     > rg.start()
     ```

- 步骤六：删除临时协调节点
  1. 连接到本地的集群管理服务进程 sdbcm

     ```lang-javascript
     > var oma = new Oma("localhost", 11790)
     ```
  2. 删除临时协调节点

     ```lang-javascript
     > oma.removeCoord(18800)
     ```

- 数据库配置启动完成
