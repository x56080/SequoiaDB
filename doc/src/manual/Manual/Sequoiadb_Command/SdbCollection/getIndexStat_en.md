##NAME##

getIndexStat - get statistics of the specified index

##SYNOPSIS##

**db.collectionspace.collection.getIndexStat\(\<index name\>\)**

##CATEGORY##

Collection

##DESCRIPTION##

Get statistics of the specified index.

##PARAMETERS##

* index name ( *String*, *Required* )

Name of the specified index.

##RETURN VALUE##

When the function executes successfully, it will return an aggregated statistics of the index through the BSONObj. Users can refer to [SDB_SNAP_INDEXSTATS](database_management/monitoring/snapshot/SDB_SNAP_INDEXSTATS.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getIndexStat()`：

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-356      |SDB_IXM_STAT_NOTEXIST|1.Index has not been analyzed; <br>2.Index doesn't exist;|1.Collect statistics by [db.analyze()](reference/Sequoiadb_command/Sdb/analyze.md); <br>2.Check if the index exists;|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

* Get the statistics of index ageIndex of collection sample.employee.

   ```
   > db.sample.employee.getIndexStat( "ageIndex" )
   ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
