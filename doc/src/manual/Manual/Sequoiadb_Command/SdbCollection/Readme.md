SdbCollection 类主要用于操作集合，包含的函数如下：

| 名称 | 描述 |
|------|------|
| aggregate() | 计算集合中数据的聚合值 |
| alter() | 修改集合的属性 |
| attachCL() | 挂载子分区集合 |
| count() | 统计当前集合符合条件的记录总数 |
| createAutoIncrement() | 创建自增字段 |
| createIdIndex() | 创建 $id 索引 |
| createIndex() | 创建索引 |
| createLobID() | 创建大对象 ID |
| deleteLob() | 删除集合中的大对象 |
| detachCL() | 从主分区集合中分离出子分区集合 |
| disableCompression() | 修改集合的属性关闭压缩功能 |
| disableSharding() | 修改集合的属性关闭分区功能 |
| dropAutoIncrement() | 删除自增字段 |
| dropIdIndex() | 删除集合中的 $id 索引 |
| dropIndex() | 删除集合中指定的索引 |
| enableCompression() | 开启集合的压缩功能或者修改集合的压缩算法 |
| enableSharding() | 修改集合的属性开启分区属性 | 
| find() | 查询记录 |
| findOne() | 查询符合条件的一条记录 |
| getDetail() | 获取集合具体信息 |
| getIndex() | 获取指定索引 |
| getIndexStat() | 获取指定索引的统计信息 |
| getLob() | 读取大对象 |
| getLobDetail() | 获取大对象被读写访问的详细信息 |
| insert() | 将单条或者批量记录插入当前集合 |
| listIndexes() | 枚举集合下的索引信息 |
| listLobs() | 列举集合中的大对象 |
| putLob() | 在集合中插入大对象 |
| remove() | 删除集合中的记录 |
| setAttributes() | 修改集合的属性 |
| split() | 切分数据记录 |
| splitAsync() | 异步切分数据记录 |
| truncate() | 删除集合内所有数据 |
| truncateLob() | 截短集合中的大对象 |
| update() | 更新集合记录 |
| upsert() | 更新集合记录 |