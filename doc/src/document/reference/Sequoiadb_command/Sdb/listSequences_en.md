##NAME##

listSequences - Enumerate sequences information

##SYNOPSIS##

**db.listSequences()**

##CATEGORY##


##DESCRIPTION##

This function is used to enumerate the sequence information of the current database.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_SEQUENCES](reference/SQL_grammar/monitoring/LIST_SEQUENCES.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, please  reference to [Troubleshooting](manual/faq.md).

##VERSION##

v3.2 and above

##EXAMPLES##

   ```
  "Name": "SYS_8589934593_id_SEQ"
}
   ```
