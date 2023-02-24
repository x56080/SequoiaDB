##NAME##

shrinkSpace - reclaim free space

##SYNOPSIS##

**db.shrinkSpace([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to reclaim the [free space][shrinkSpace] of all files under the specified collection space.

##PARAMETERS##

options ( *object, optional* )

Modify the collection space to be reclaimed and the command position parameters through the parameter "options":

- CollectionSpace ( *string* ): Collection space name.

    If this parameter is not specified, all free space under the collection space will be reclaimed by default.

    Format: `CollectionSpace: "sample"`

-  Location Elements ( *object* ): [Command positional parameters][location].

    If this parameter is not specified, it will be executed globally by default.

    Format: `GroupName: "db1"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `shrinkSpace()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -34    | SDB_DMS_CS_NOTEXIST | Collection space does not exist. | Check whether the specified collection space exists. |
| -396   | SDB_SYSENV_NOT_SUPPORT | The system environment does not support constructing file holes. | Replace file system to ext4(since Linux 3.0). |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.4 and above

##EXAMPLES##

Reclaim the free space under the collection space "sample".

```lang-javascript
> db.shrinkSpace({CollectionSpace: "sample"})
```

[^_^]:
    Links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[shrinkSpace]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md#空间回收