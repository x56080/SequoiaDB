##概念##

协调节点为一种逻辑节点，其中并不保存任何用户数据信息。

协调节点作为数据请求部分的协调者，本身并不参与数据的匹配与读写操作，而仅仅是将请求分发到所需要处理的数据节点。

一般来说，协调节点的处理流程如下：

- 得到请求
- 解析请求
- 本地缓存查询该请求对应集合的信息
- 如果信息不存在则从编目节点获取
- 将请求转发至相应的数据节点
- 从数据节点得到结果
- 把结果汇总或直接传递给客户端

协调节点与其它节点之间主要使用分区服务端口（ SequoiaDB 的 --shardname 参数）进行通讯。

SequoiaDB 中有两类协调节点：

1. 临时协调节点：通过资源管理节点 sdbcm 建立的协调节点。临时协调节点并不会注册到编目节点中，即该临时的协调节点不能被集群管理。临时协调节点仅用于初始创建 SequoiaDB 集群使用。
2. 协调节点：通过正常的流程创建的协调节点组中的协调节点。该类协调节点注册到编目节点中，并且可以被集群管理。

##管理协调节点##

###创建临时协调节点###

创建 SequoiaDB 集群时，可以在 Sdb Shell 中通过 sdbcm 创建临时协调节点：

1. 连接到本地的集群管理服务进程 sdbcm

   ```lang-javascript
   > var oma = new Oma( "localhost", 11790 )
   ```

2. 创建临时协调节点

   ```lang-javascript
   > oma.createCoord( 18800, "/opt/sequoiadb/database/coord/18800" )
   ```

3. 启动临时协调节点

   ```lang-javascript
   oma.startNode( 18800 )
   ```
>   **Note:**
>
>   更详细的创建临时协调节点，请参考 [Oma.createCoord\(\)](reference/Sequoiadb_command/Oma/createCoord.md)


###创建协调节点组###

在 Sdb Shell 中可以通过临时协调节点可以创建协调节点组：

1. 连接临时协调节点

   ```lang-javascript
   > var db = new Sdb( 'sdbserver1', 18800 )
   ```

2. 创建协调节点组

   ```lang-javascript
   > db.createCoordRG()
   ```

>   **Note:**
>
>   协调节点组中的协调节点将会注册到编目节点中，并被集群管理。因此创建协调节点组前应先在集群中创建有效的编目节点。
>
>   如何创建协调节点组和部署协调节点可以详细请参考 [集群模式](installation/deployment/command_installation/cluster.md)
>
>   更详细的创建协调节点组，请参考 [Sdb.createCoordRG\(\)](reference/Sequoiadb_command/Sdb/createCoordRG.md)

###新增协调节点###

当集群规模扩大时，协调节点也需要随着规模的增加而进行增加。建议匹配时，一台物理节点，配置一个协调节点。

在 Sdb Shell 中可以通过现有的协调节点组添加新的协调节点（假设 sdbserver1 中已有协调节点或临时协调节点，现在向 sdbserver2 中添加新的协调节点）：

1. 连接 sdbserver1 的协调节点 11810

   ```lang-javascript
   > var db = new Sdb( 'sdbserver1', 11810 )
   ```

2. 获取协调节点组

   ```lang-javascript
   > var rg = db.getCoordRG()
   ```

   >   **Note:**
   >
   >   在 Sdb Shell 中也可以使用 (Sdb.getRG\(\))[reference/Sequoiadb_command/Sdb/getRG.md] 以 "SYSCoord" 为分>
区组组名获取编目节点组。

3. 在 sdbserver2 中新建协调节点 11810

   ```lang-javascript
   > var node = rg.createNode( "sdbserver2", 11810, "/opt/sequoiadb/database/coord/11810" )
   ```

4. 启动 sdbserver2 的协调节点

   ```lang-javascript
   > node.start()
   ```

###查看协调节点###

在 Sdb Shell 中可以查看协调节点的列表：

```lang-javascript
> db.getCoordRG().getDetail()
```

###手工创建协调节点###

1. 创建协调节点配置目录，其中11810为协调节点的服务端口，可根据需要配置；

  ```lang-javascript
  $ mkdir -p /opt/sequoiadb/conf/local/11810
  ```

2. 拷贝协调节点样例配置文件；

  ```lang-javascript
  $cp /opt/sequoiadb/conf/samples/sdb.conf.coord /opt/sequoiadb/conf/local/11810/sdb.conf
  ```

3. 修改配置文件；

  ```lang-javascript
  $ vi /opt/sequoiadb/conf/local/11810/sdb.conf
  ```

  修改内容

  ```
  # database path dbpath=/opt/sequoiadb/database/coord
  ```

  该参数为数据库放置路径，可根据需要修改，请确保路径已经存在（不存在请手工创建）

  将如下行：

  ```
  # catalog addr(hostname1:servicename1,hostname2:servicename2,...)
  # catalogaddr=
  ```

  修改为:

  ```
  # catalog addr(hostname1:servicename1,hostname2:servicename2,...)
  catalogaddr=sdbserver1:11803,sdbserver2:11803,sdbserver3:11803
  ```

  该参数为Catalog服务地址和端口

4. 按 :wq，保存退出 vi；

5. 创建数据文件存放路径，路径为上一步骤配置的路径；

  ```lang-javascript
  $ mkdir -p /opt/sequoiadb/database/coord
  ```

6. 启动协调节点进程。

  ```lang-javascript
  $ /opt/sequoiadb/bin/sdbstart -c /opt/sequoiadb/conf/local/11810/
  ```

##故障恢复##

由于协调节点不存在用户数据，因此发生故障后可以直接重新启动，不参与任何额外的故障恢复步骤。
