此部分是相关 [C++ API](api/cpp/html/index.html) 文档。

##历史更新情况：##

**注意：** 

* 删除接口 - 不再兼容 
* 废弃接口 - 保持兼容性

**Version 2.10**

1. sdbCollection 类添加接口

   * enableSharding，对集合启用分区功能
	* disableSharding，对集合关闭分区功能
	* enableCompression，对集合启用压缩功能
	* disableCompression，对集合关闭压缩功能
	* setAttributes，对集合的属性进行修改

2. sdbCollectionSpace 类添加接口

   * alterCollectionSpace，对集合空间的属性进行修改
	* setAttributes，对集合空间的属性进行修改
	* setDomain，修改集合空间所属的域
	* removeDomain，移除集合空间所属的域

3. sdbDomain 类添加接口

	* addGroups，向域中添加数据组
	* setGroups，对域设置数据组
	* removeGroups，移除属于域的某些数据组
	* setAttributes，设置域的属性

**Version 2.9**

1. sdbReplicaGroup类接口变更：

	* 废弃getNodeNum接口，该接口描述的节点状态信息不准确。

2. sdbNode类接口变更：

	* 废弃getStatus接口，该接口描述的节点状态信息不准确。

**Version 2.6**

1. sdb 类接口变更：

	* 添加getLastAliveTime接口，获取sdb最后与服务器端交互时间到标准计时点的秒数。该接口主要用于C++驱动连接池。


**Version 1.10**

1. sdbCollection 类接口变更：

	增加如下接口：

	* explain，获取查询的访问计划。
	* createLob，创建一个新的lob。
	* openLob，打开一个已存在的lob，该版本中，打开的lob只用于读操作。
	* removeLob，删除一个lob。
	* listLobs，列出当前collection中的所有lob。

2. sdbLob 类接口变更：

	增加如下接口：

	* read，从lob中读取数据。
	* write，把数据写入lob中。
	* seek，设置读起始位置，该版本中，seek只用于读操作。
	* close，关闭一个新创建的或打开的lob。
	* getOid，获取lob的oid。
	* getSize，获取lob的大小。
	* getCreateTime，获取lob的创建时间。

**Version 1.8**

1. sdb 类接口变更：

	新增如下接口：

	* connect，可提供多个地址，接口随机选择一个有效的地址连接。
	* createCollectionSpace，提供一个 BSONObject 的选项，使创建集合空间更加灵活。
	* backup，备份支持更多的选项。
	* createDomain，创建域。
	* getDomain，获取域。
	* dropDomain，删除域。
	* listDomain，列出所有域。

2. sdbCollection 类接口变更：

	* alterCollection，修改集合（表）属性


3. 新增加sdbDomain类用于域操作。

**Version 1.6**

1. 添加类 Node 来取代原来的类 ReplicaNode。类 ReplicaNode 以及与它相关的方法将在 version 2.x 中被弃用。

