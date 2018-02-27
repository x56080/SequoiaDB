此部分是相关[Python API](api/python/html/index.html)文档。

##历史更新情况：##

**Version 2.8.5**

client 新增接口：

-  get_session_attri，获取会话设置属性

**Version 2.8**

（1）client 新增接口：

-  list_domains，查看域列表
-  is_domain_existing，查看域是否存在
-  create_domain，创建域
-  drop_domain，删除域
-  get_domain，获取域对象
-  get_cata_replica_group，获取编目节点组对象
-  get_coord_replica_group，获取协调节点组对象
-  create_cata_replica_group，创建编目节点组
-  create_coord_replica_group，创建协调节点组
-  remove_cata_replica_group，删除编目节点组
-  remove_coord_replica_group，删除协调节点组
-  start_replica_group，启动复制组
-  stop_replica_group，停止复制组

（2）新增接口类 domain：

-  alter，修改域属性
-  list_collection_spaces，查看属于该域的集合空间列表
-  list_collections，查看属于该域的集合列表

**Version 1.10**

（1） 新增接口类 lob：

-   close，关闭创建的lob对象，用以刷新数据
-   read，可从lob对象中读取数据
-   write，可把数据写入lob
-   seek，可跳转到到指定数据位置
-   get_oid，可获取lob对象的oid
-   get_size，可获取lob对象的大小(bytes)
-   get_create_time，可获取lob对象的创建时间

（2） collection 新增接口：

-   create_lob，可在当前的collection中创建一个lob对象
-   remove_lob，可在当前的collection中删除指定lob对象
-   get_lob，可获取当前collection中指定oid的lob对象
-   list_lobs，可列出当前collection中所有的lob

详情请查看相关[Python API](api/python/html/index.html)。
