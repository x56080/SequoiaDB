此部分是相关 [CSharp API](api/cs/html/index.html) 文档。



##历史更新情况：##

**注意：** 

* 删除接口 - 不再兼容 
* 废弃接口 - 保持兼容性

**Version 2.10**

1. SequoiaDB::DBCollection接口变更：

	* 增加OpenLob(ObjectId id, int mode)方法，其中mode取值为DBLob.SDB_LOB_READ或DBLob.SDB_LOB_WRITE。
	* 增加enableSharding方法，对集合启用分区功能
	* 增加disableSharding方法，对集合关闭分区功能
	* 增加enableCompression方法，对集合启用压缩功能
	* 增加disableCompression方法，对集合关闭压缩功能
	* 增加setAttributes方法，对集合的属性进行修改

2. SequoiaDB::DBLob接口变更：

	* 增加Lock方法。
 	* 增加LockAndSeek方法。
	* 增加GetModificationTime方法。
	* Seek方法原来只能在读lob模式下使用，现在该方法支持在创建的lob或写lob模式下使用。

3. SequoiaDB::CollectionSpace接口变更

   * 增加alter方法，对集合空间的属性进行修改
	* 增加setAttributes方法，对集合空间的属性进行修改
	* 增加setDomain方法，修改集合空间所属的域
	* 增加removeDomain方法，移除集合空间所属的域

4. SequoiaDB::Domain接口变更

	* 增加addGroups方法，向域中添加数据组
	* 增加setGroups方法，对域设置数据组
	* 增加removeGroups方法，移除属于域的某些数据组
	* 增加setAttributes方法，设置域的属性

**Version 2.9**

1. 类SequoiaDB::Sequoiadb接口变更：

	* 增加sync方法控制数据持久化。

2. SequoiaDB::ReplicaGroup接口变更：

	* 废弃GetNodeNum接口，该接口描述的节点状态信息不准确。

3. SequoiaDB::Node接口变更：

	* 废弃GetStatus接口，该接口描述的节点状态信息不准确。


**Version 1.12**

1. 添加使用SSL连接数据库的接口