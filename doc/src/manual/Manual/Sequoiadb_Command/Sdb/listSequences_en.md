

##NAME##

listSequences - Show the sequences name of the current database.

##SYNOPSIS##

***db.listSequences()***

##CATEGORY##

Sdb

##DESCRIPTION##

Show the sequences name of the current database.

>**Note:**

>db.listSequences() can only show the sequences name. If you want to show detail information of the sequences, please refer to [db.snapshot( SDB_SNAP_SEQUENCES )](manual/Manual/Snapshot/SDB_SNAP_SEQUENCES.md).

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the sequences name of the current database.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, please  reference to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Show the sequences name of the current database.

```lang-javascript
> db.listSequences()
{
  "Name": "SYS_8589934593_id_SEQ"
}
```
