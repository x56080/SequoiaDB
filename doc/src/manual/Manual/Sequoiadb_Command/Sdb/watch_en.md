##NAME##

watch - Subscribe the change stream of the node

##SYNOPSIS##

**Sdb.watch( [token], [options] )**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to subscribe the change stream of the node.

##PARAMETERS##

* token (*StreamToken, optional*)

    Specifies the starting position of this change stream.

* options (*object, optional*)

    - CollectionSpaces ( *string array* )

        The collection spaces want to be watched. If no CollectionSpaces or Collections specified, all collection spaces will be watched.

        Format: `CollectionSpaces: [ "foo" ]`

    - Collections ( *string array* )

        The collections want to be watched. If no CollectionSpaces or Collections specified, all collections will be watched.

        Format: `Collections: [ "foo.bar" ]`

    - ChangeTypes (*string*)

        The type of change events to subscribe to. Supported types include "RECORD" (BSON record DML operations), "LOB" (LOB DML operations), "DDL" (DDL operations), "TRANS" (transaction change operations), "ALL" (all types). Multiple types can be connected with "|", and it is not case sensitive. The default is: "RECORD|DDL".

        Format: `ChangeTypes: "RECORD|DDL"`

    - MaxWaitTime (*integer*)

        Specifies the maximum waiting time when there is no new data, in seconds. After the timeout, the change stream will return an empty control type record. Less than 0 means waiting indefinitely, 0 means no waiting. Default: 1 second.

        Format: `MaxWaitTime: 1`

    - CacheSize (*integer*)

        The amount of log data to cache, in MB. The range is 0~2048, where 0 means no cache setting, and the maximum is 2 GB. Default: 32. Logs that exceed the cache size will need to be read from the log file.

        Format: `CacheSize: 32`

##RETURN VALUE##

If the function executes successfully, it will return an SdbCursor type object. The returned change stream data can be obtained through this object.

If the function execution fails, it will throw an exception and output error information.

##ERRORS##

Common exceptions of the `watch()` function include:

| Error Code | Error Type | Possible Cause | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | The filled parameter is wrong, invalid parameters, etc., and detailed error information description will be returned | Fill in the correct parameters |
| -150 | SDB_RTN_IN_REBUILD | The node is being rebuilt | Subscription needs to wait until the full synchronization is over |
| -129 | SDB_CLS_FULL_SYNC | The node is synchronizing fully | Subscription needs to wait until the full synchronization is over |
| -34 | SDB_DMS_CS_NOTEXIST | The specified subscription collection space does not exist | Check if the collection space exists |
| -23 | SDB_CS_NOTEXIST | The specified subscription collection does not exist | Check if the collection exists |
| -404 | SDB_STREAM_NOT_RESUMABLE | The change stream is not resumable, the current log LSN and the LSN of the change stream recovery are too far apart | Consider full synchronization, or increase the node's [configuration parameter][config] changestreamresumewindow |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v7.2.2 and above

##EXAMPLES##

```lang-javascript
// Create a StreamToken object
var token = new StreamToken();

// Set subscription options
var options = {
  ChangeTypes: "RECORD|DDL",
  MaxWaitTime: 2,
  CacheSize: 64
};

// Subscribe to the change stream
var cursor = db.watch(token, options);

// Get and print the change stream data
while (cursor.next()) {
  println(cursor.current());
}
```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[config]:manual/Manual/Database_Configuration/configuration_parameters.md