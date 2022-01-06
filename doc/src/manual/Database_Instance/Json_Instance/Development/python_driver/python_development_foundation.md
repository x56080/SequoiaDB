本文档主要介绍如何使用 Python 客户端驱动接口编写使用 SequoiaDB 巨杉数据库的程序。下述为 SequoiaDB 巨杉数据库 Python 驱动的简单示例，示例中的代码可能不完整，用户可在 `/sequoiadb/samples/Python` 目录下获取相应的完整代码。

> **Note:** 
> 
> 在 Python 中构造 BSON 时默认使用 dict，dict 的字段是无序的。如果要求 BSON 中的字段顺序与输入顺序一致（例如，创建索引时索引键的定义），需要使用 collections.OrderedDict。

##数据库操作##

* 数据库连接

   通过编写 `connect.py` 连接到数据库
  
   ```lang-python
   from pysequoiadb import *

   host = 'localhost'
   port = 11810
   host_list = [ {'host': 'sdbserver1', 'service': 11810}, { 'host': 'sdbserver2', 'service': 11810 } ]

   # 只使用一个地址连接
   db = client( host, port )
   db.disconnect()

   # 使用多个地址连接，从地址列表中随机选择地址进行连接，直至连接成功
   db = client( host_list=host_list, policy='random' )
   db.disconnect()

   # 如果数据库已经创建用户，需要使用正确的用户及密码才能连接到数据库
   user = "sdbadmin"
   psw = "sdbadmin"
   db = client( host, port, user, psw  )
   db.disconnect()

   # 当然，也支持使用密码文件进行连接
   cipher_file="/opt/sequoiadb/cipher"
   db = client( host, port, user, cipher_file=cipher_file )
   db.disconnect()
   ```

   > **Note:**
   >
   > - 用户可以根据实际情况调整上述配置参数，如用户名、密码等。
   >
   > - 密码文件的使用，请参考[密码管理工具](manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md)

* 创建集合空间和集合
  
   ```lang-python
   # 连接至数据库
   db = client("localhost", 11810)
   # 创建集合空间
   cs_name = 'sample'
   cs = db.create_collection_space(cs_name)
   # 创建集合
   cl_name = 'employee'
   cl = cs.create_collection(cl_name)
   ```

   用户创建集合后，可对集合做增删改查等操作

* 插入数据

   ```lang-python
   # 创建 dict 对象
   record = {"name":"Tom", "age":24}
   oid = cl.insert ( record ) 
   ```
  
   record 为输入参数，输入需要插入的数据。dict 对象将会被转换成 bson 插入到集合中。oid 是插入该记录时，返回的 bson 结构的 objectid。
  
* 查询

   查询操作需要一个游标对象存放查询的结果到本地。要获得查询的结果需要使用游标操作。本示例使用了游标操作的 next 接口，表示从查询结果中取到一条记录。

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

* 索引

  集合对象 collection 中创建一个以“name”为升序、“age”为降序的索引

   ```lang-python
   index_name = "index_name"
   idx = OrderedDict([('name', 1), ('age', -1)])
   cl.create_index ( idx, index_name, False, False ) 
   ```
  
* 更新

   在集合对象 collection 中更新了记录
 
   ```lang-python
   rule = {"$set":{ "age":19}}
   print rule
   cl.update( rule )
   ```
  
   示例中没有指定数据匹配规则，所以将更新集合中的所有记录。
  
* 错误处理

   调用 API 遇到的异常时，python 驱动会将异常直接抛出。可以选择捕获异常，并打印异常信息或是进行一些其他操作。SDBBaseError 异常是基础异常，异常主要包含 errcode、detail 和 error_object。异常的详情可以查询 [Python API](api/python/html/index.html)。示例如下：

   ```lang-python
   try:
       cl = db.get_collection("sample.employee")
	  	  condition = {"_id":{"$oid":"5d035e2bb4d450b04fcd0dff"}}
	  	  cl.delete ( condition=condition )
   except SDBBaseError as e:
	    print(e)
   ```

    异常信息如下：

   ```lang-text
   SDB_INVALIDARG(-6), Invalid Argument, detail: Failed to delete
   ```

##集群操作##

* 复制组操作

   复制组操作包括创建复制组（client::creat_replica_group）、得到复制组实例（client:: get_replica_group_by_name 和 client:: get_replica_group_by_id）、启动复制组所有节点（replicagroup::start）、停止复制组所有节点（replicagroup::stop）等。以下仅作为示例，真正的应用应包括错误检测等。

   ```lang-python
   # 创建名为“group1”的数据组
   rg = db.create_replica_group ("group1")

   # 创建节点时，定义一个空的 map 对象 config 表示该节点没有更多的配置内容
   config = {}
   rg.create_node ('sdbserver', '11820', "/opt/sequoiadb/database/11820", config)

   # 启动复制组
   rg.start ()
   ```


* 节点操作

   节点操作包括创建节点（replicagroup::create_node）、获取主节点（replicagroup::get_master）、获取备节点（replicagroup::get_slave）、启动节点（replicanode::start）、停止节点（replicanode::stop）等。
  
   以下为数据节点操作示例性的例子，真正的应用应包括错误检测等。
  
   ```lang-python
   # 获取数据组 group1
   rg = db.get_replica_group_by_name("group1")
  
   # 获取数据主节点
   master = rg.get_master() 
  
   # 获取数据备节点
   slave = rg.get_slave() 
   ```
