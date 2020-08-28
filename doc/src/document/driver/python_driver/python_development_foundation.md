本节介绍使用 Python 运行 SequoiaDB。首先安装 SequoiaDB，安装信息请查看[安装](installation/deployment/visualization_installation.md)章节。

这里介绍如何使用 Python 客户端驱动接口编写使用 SequoiaDB 数据库的程序。为了简单起见，下面的示例不全是完整的代码，只起示例性作用。可到 SequoiadDB 安装路径下 `samples/Python` 下获取相应的完整的示例代码。查看 [Python API](api/python/html/index.html) 可获取更多接口信息。

> Note:  
> 在 Python 中构造 BSON 时默认使用 dict，dict 的字段是无序的。
> 如果要求 BSON 中的字段顺序与输入顺序一致（例如，创建索引时索引键的定义），请使用 collections.OrderedDict。

##数据库操作##

* 数据库连接

  以下是数据库连接示例代码，演示如何连接到数据库。
  
  ```lang-python
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

  在 Linux 下，可以直接运行 python 解释执行上述示例代码。

  > Note:

  > 本例子连接到本地数据库的服务端口 11810，使用的是空的用户名和密码。用户需要根据自己的实际情况配置参数。譬如，需要连接目标主机 sdbserver 上的数据库时，则需要将上述的 host 设置为 'sdbserver'。当数据库已经创建用户时，应该使用正确的用户及密码连接到数据库，否则将会连接失败。

* 创建集合空间和集合

  以下创建了一个名字为 “sample” 的集合空间和一个名字为 “employee” 的集合，集合空间内的集合的数据页大小为 16k。可根据实际情况选择不同大小的数据页。创建集合后，可对集合做增删改查等操作。
  
  ```lang-python
  # connect to db
  db = client("localhost", 11810)
  
  # create collection space
  cs_name = 'sample'
  cs = db.create_collection_space(cs_name)
  
  # create collection
  cl_name = 'employee'
  cl = cs.create_collection(cl_name)
  ```

* 插入数据

  ```lang-python
  # creat dict object
  record = {"name":"Tom", "age":24}
  oid = cl.insert ( record ) ;
  ```
  
  record 为输入参数，为要插入的数据。dict 对象将会被转换成 BSON 插入到集合中。oid 是插入该记录时，返回的 BSON 结构的 ObjectId。
  
* 查询

  ```lang-python
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
  cr.close()
  ```

  查询操作需要一个游标对象存放查询的结果到本地。要获得查询的结果需要使用游标操作。本例使用了游标操作的next接口，表示从查询结果中取到一条记录。此示例中没有设置查询条件，筛选条件，排序情况，及仅使用默认索引。

* 索引

  ```lang-python
  index_name = "index_name"
  idx = OrderedDict([('name', 1), ('age', -1)])
  cl.create_index ( idx, index_name, False, False ) ;
  ```
  
  在集合对象 cl 中创建一个以 “name” 为升序，“age” 为降序的索引。
  
* 更新

  ```lang-python
  rule = {"$set":{ "age":19}}
  print rule
  cl.update( rule )
  ```
  
  在集合对象 cl 中更新了记录。实例中没有指定数据匹配规则，所以此示例将更新集合中所有的集合。
  
* 错误处理

  调用 API 遇到异常时，python 驱动会将异常直接抛出。可以选择捕获异常，并打印异常信息或是进行一些其他操作。SDBBaseError 异常是基础异常，异常主要包含 errcode、detail 和 error_object。示例如下：

  ```lang-python
  try:
      cl = db.get_collection("sample.employee")
 	  condition = {"_id":{"$oid":"5d035e2bb4d450b04fcd0dff"}}
 	  cl.delete ( condition=condition )
  except SDBBaseError as e:
	   print(e)
  ```
  上述示例的输出结果：

  ```lang-text
	SDB_INVALIDARG(-6), Invalid Argument, detail: Failed to delete
  ```

##集群操作##

分区组操作包括创建分区组（creat_replica_group），得到分区组实例（ get_replica_group_by_name 和 get_replica_group_by_id），启动分区组所有节点（start），停止分区组所有节点（stop）等。

* 分区组操作

  以下仅作为示例，真正的应用应包括错误检测等。

  ```lang-python
  rg = db.create_replica_group ("group1")
  config = {}
  rg.create_node ('sdbserver', '11820', "/opt/sequoiadb/database/data/11820", config)
  rg.start ()
  ```

  创建名为 group1 的数据组。创建节点时，定义一个空的 dict 对象 config 表示该节点没有更多的配置内容。

* 节点操作

  节点操作包括创建节点（create_node），获取主节点（get_master），获取备节点（get_slave），启动节点（start），停止节点（stop）等。
  
  以下为数据节点操作示例性的例子。真正的应用应包括错误检测等。
  
  ```lang-python
  # 获取数据组group
  rg = db.get_replica_group_by_name("group1")
  
  # 获取数据主节点
  master = rg.get_master() ;
  
  # 获取数据备节点
  slave = rg.get_slave() ;
  ```
