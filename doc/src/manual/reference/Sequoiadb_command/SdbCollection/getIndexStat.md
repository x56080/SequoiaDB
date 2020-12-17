##名称##

getIndexStat - 获取指定索引的统计信息

##语法##

**db.collectionspace.collection.getIndexStat\(\<index name\>\)**

##类别##

Collection

##描述##

该函数用于获取当前集合中指定索引的统计信息。

##参数##

* index name ( *string*， *必填* )

被指定索引的名称。

##返回值##

函数执行成功时，返回汇总后的索引统计信息，其类型为 BSONObj。返回的字段信息可参考[索引统计信息快照](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_INDEXSTATS.md)。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getIndexStat()`函数常见异常如下：

|错误码|错误名|可能发生的原因|解决办法|
|------|------|--------------|--------|
|-349  |SDB_IXM_STAT_NOTEXIST|1.索引尚未被统计<br>2.索引不存在|1.通过 [db.analyze()](reference/Sequoiadb_command/Sdb/analyze.md) 接口收集统计信息<br>2.检查索引是否存在|

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](manual/faq.md)。

##版本##

v3.4.2 及以上版本

##示例##

* 获取集合 sample.employee 中 ageIndex 索引的统计信息

```lang-javascript
> db.sample.employee.getIndexStat( "ageIndex" )
```

结果如下：

```lang-text
{
  "Collection": "sample.employee",
  "Index": "ageIndex",
  "Unique": false,
  "KeyPattern": {
    "age": 1
  },
  "TotalIndexLevels": 1,
  "TotalIndexPages": 2,
  "DistinctValNum": [
    74
  ],
  "MinValue": {
    "age": 18
  },
  "MaxValue": {
    "age": 54
  },
  "NullFrac": 0,
  "UndefFrac": 0,
  "SampleRecords": 400,
  "TotalRecords": 518,
  "StatTimestamp": "2020-07-24-16.15.08.347000"
}
```
