本节介绍使用Python运行SequoiaDB。首先安装SequoiaDB，安装信息请查看[安装](installation/deployment/visualization_installation.md)章节。

这里介绍如何使用Python客户端驱动接口编写使用SequoiaDB数据库的程序。为了简单起见，下面的示例不全是完整的代码，只起示例性作用。可到SequoiadDB安装路径下samples/Python下获取相应的完整的代码。更多查看[Python API](api/python/html/index.html)

> Note:  
> 在Python中构造BSON时默认使用dict，dict的字段是无序的。  
> 如果要求BSON中的字段顺序与输入顺序一致（例如，创建索引时索引键的定义），请使用collections.OrderedDict。

##数据库操作##

* 数据库连接（Connecting）

  以下是connect.py演示如何连接到数据库。
  
  ```lang-javascript
  import pysequoiadb
  from pysequoiadb import client

  # connect to local db, using default args value.
  host = 'localhost'
  port = 11810
  # user= '', password= ''
  db = client(host, port)
  
  # if no error occurs, connect to specified server successfully
  print 'Connect success'
  db.disconnect()
  ```

  在Linux下，可以直接运行python解释执行connect.py。

  > Note:

  > 本例程连接到本地数据库的服务端口11810，使用的是空的用户名和密码。用户需要根据自己的实际情况配置参数。譬如，将上述代码中的 `db = client()` 修改为 `db = client('192.168.10.188', 11810)`。当数据库已经创建用户时，应该使用正确的用户及密码连接到数据库，否则连接失败。

* 创建集合空间和集合

  以下创建了一个名字为“foo”的集合空间和一个名字为“bar”的集合，集合空间内的集合的数据页大小为16k。可根据实际情况选择不同大小的数据页。创建集合后，可对集合做增删改查等操作。
  
  ```lang-javascript
  # connect to db
  db = client("localhost", 11810)
  
  # create collection space
  cs_name = 'foo'
  cs = db.create_collection_space(cs_name)
  
  cl_name = 'bar'
  cl = cs.create_collection(cl_name)
  ```

* 插入数据（insert）

  ```lang-javascript
  # creat dict object
  record = {"name":"Tom", "age":24}
  oid = cl.insert ( record ) ;
  ```
  
  record为输入参数，为要插入的数据。dict对象将会被转换成bson插入到集合中。oid 是插入该记录时，返回的bson结构的objectid。
  
* 查询（query）

  ```lang-javascript
  import pysequoiadb
  from pysequoiadb import client
  from pysequoiadb.error import SDBEndOfCursor

  cr = cl.query()
  while True:
     try:
        record = cr.next()
        print(record) 
     except SDBEndOfCursor:
        break
     finally:
        cr.close()
  ```

  查询操作需要一个游标对象存放查询的结果到本地。要获得查询的结果需要使用游标操作。本例使用了游标操作的next接口，表示从查询结果中取到一条记录。此示例中没有设置查询条件，筛选条件，排序情况，及仅使用默认索引。

* 索引（index）

  ```lang-javascript
  index_name = "index_name"
  idx = OrderedDict([('name', 1), ('age', -1)])
  cl.create_index ( idx, index_name, False, False ) ;
  ```
  
  集合对象collection中创建一个以“name”为升序，“age”为降序的索引。
  
* 更新（update）

  ```lang-javascript
  rule = {"$set":{ "age":19}}
  print rule
  cl.update( rule )
  ```
  
  在集合对象 ollection中更新了记录。实例中没有指定数据匹配规则，所以此示例将更新集合中所有的集合。
  
##集群操作##

分区组操作包括创建分区组（client::creat_replica_group），得到分区组实例（client:: get_replica_group_by_name 和 client:: get_replica_group_by_id），启动分区组所有节点（replicagroup::start），停止分区组所有节点（replicagroup::stop）等。

* 分区组操作

  以下仅作为示例，真正的应用应包括错误检测等。

  ```lang-javascript
  rg = db.create_replica_group ("group1")
  
  config = {}
  rg.create_node ('ubuntu-test-03', '20000', "/opt/sequoiadb/database/20000", config)
  
  rg.start ()
  ```

  创建名为group1的数据组。创建节点时，定义一个空的map对象config表示该节点没有更多的配置内容。

* 节点操作

  节点操作包括创建节点（replicagroup::create_node），获取主节点（replicagroup::get_master），获取备节点（replicagroup::get_slave），启动节点（replicanode::start），停止节点（replicanode::stop）等。
  
  以下为数据节点操作示例性的例子。真正的应用应包括错误检测等。
  
  ```lang-javascript
  # 获取数据组group
  rg = db.get_replica_group_by_name("group1")
  
  # 获取数据主节点
  master = rg.get_master() ;
  
  # 获取数据备节点
  slave = rg.get_slave() ;
  ```
