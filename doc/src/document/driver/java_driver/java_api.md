此部分是相关 [Java API](api/java/html/index.html) 文档。

##历史更新情况##

**注意：** 

* 删除接口 - 不再兼容 
* 废弃接口 - 保持兼容性

**Version 2.8.5**

1. com.sequoiadb.base.Sequoiadb内容变更：

   * setSessionAttr接口增加可以选择多个 Instance 的功能。

2. com.sequoiadb.base.Domain内容变更：

   * 修复isDomainExist接口可能存在游标泄露的情况（SEQUOIADBMAINSTREAM-3264）。
   
3. com.sequoiadb.datasource.DatasourceOptions内容变更：

   * setSyncCoordInterval(int syncCoordInterval)接口正常的输入参数syncCoordInterval 的值若小于60,000，该接口自动将输入值改为60,000。
   * 增加getPreferedInstance/setPreferedInstance接口，使连接池支持设置回话属性。

**Version 1.10**

1. com.sequoiadb.base.DBCollection内容变更：

	* 增加createLob方法，用于创建一个大对象。
	* 增加openLob方法，用于打开一个已存在的大对象。
	* 增加removeLob方法，用于删除一个大对象。
	* 增加listLobs方法，用于列出所有大对象。
	* 增加explain方法，用于获取执行访问计划。

2. 增加com.sequoiadb.base.DBLob，用于操作大对象：

	该类包含如下接口：

	* write，向一个大对象写入数据。
	* read，从大对象中读取数据。
	* seek，指定读取数据的偏移。
	* close，关闭一个大对象。
	* getID，获取大对象的标识ID。
	* getSize，获取大对象的大小。
	* getCreateTime，获取大对象的创建时间。

**Version 1.8**

1. com.sequoiadb.base.Sequoiadb 类添加如下接口：

	* isValid，判断当前连接是否有效。
	* createCollectionSpace，提供一个 BSONObject 的选项，使创建集合空间更加灵活。
	* backupOffline，离线备份支持更多的选项。
	* evalJS，执行 js 代码。
	* createDomain，创建域。
	* getDomain，获取域。
	* dropDomain，删除域。
	* isDomainExist，域是否存在。
	* listDomain，列出所有域。

2. com.sequoiadb.base.DBCollection 类添加如下接口：

	* alterCollection，修改集合（表）属性。
	* setMainKeys，设置主键。此接口只与 save 接口配合使用，它设置的主键并不对其他接口起作用。
	* save，可使用默认的主键"_id"或者指定其他主键，同时插入或更新多条记录。

3. 添加 com.sequoiadb.base.Domain 类用于与域相关的操作。

4. com.sequoiadb.base.SequoiadbDatasource 类添加如下接口：

	* SequoiadbDatasource，可提供多个地址的构造器，便于机器负载均衡。
	* getIdleConnNum，获取当前可用的连接数量。
	* getUsedConnNum，获取当前已使用的连接数量。
	* getNormalAddrNum，获取当前正常的地址数量。
	* getAbnormalAddrNum，获取当前异常的地址数量。

5. com.sequoiadb.base.SequoiadbOption 类新添加接口：

	* setRecaptureConnPeriod，设置周期检测异常地址是否重新可用的时间。
	* getRecaptureConnPeriod，获取周期检测异常地址是否重新可用的时间。

**Version 1.6**

1. 添加类 Node 来取代原来的类 ReplicaGroup。类 ReplicaNode 以及与它们相关的方法将在 version 2.x 中被弃用。
