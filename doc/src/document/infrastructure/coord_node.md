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

协调节点与其它节点之间主要使用分区服务端口（ shardname 参数）进行通讯。

##新增协调节点##

当集群规模扩大时，协调节点也需要随着规模的增加而进行增加。建议匹配时，一台物理节点，配置一个协调节点。

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
