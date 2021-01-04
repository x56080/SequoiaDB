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
|-349      |SDB_IXM_STAT_NOTEXIST|1.Index has not been analyzed; <br>2.Index doesn't exist;|1.Collect statistics by [db.analyze()](reference/Sequoiadb_command/Sdb/analyze.md); <br>2.Check if the index exists;|

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.4.2 and above

##EXAMPLES##

* Get the statistics of index ageIndex of collection sample.employee.

   ```
   > db.sample.employee.getIndexStat( "ageIndex" )
   ```
