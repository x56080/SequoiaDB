##NAME##

getDetail - get detailed information of the current collection

##SYNOPSIS##

**db.collectionspace.collection.getDetail\(\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

Get detailed information of the current collection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of current collection through the cursor. Users can refer to [SDB_SNAP_COLLECTIONS][SNAPSHOT_CL] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2.5 and above, v3.4.1 and above

##EXAMPLES##

* Get the detail of collection `sample.employee`.

   ```
   > db.sample.employee.getDetail()
   ```


[^_^]:
     links
[SNAPSHOT_CL]:manual/Manual/SQL_grammar/monitoring/SNAPSHOT_CL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
