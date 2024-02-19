##NAME##

invalidateFsCache - clear the file system cache of the nodes

##SYNOPSIS##

**db.invalidateFsCache( [options], [expiredTime] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to clear the file system cache of the catalog or data nodes.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| options | json   | Specify [command positional parameter][location]. When null, clear the cache of the catalog, data or coord nodes. | Not |
| expiredTime | String | The expired time of cache. The cache that out of this time would be cleared. String format is a integer or a integer end with time unit. Time unit: hour(h), minute(m) or second(s). If it is a integer without unit, it equals unit 'h'. e.g.: "72h". And "" is expired immediately. Default: "". | Not |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.2 and above

##EXAMPLES##

* Clear the file system cache of all nodes of current cluster.

```lang-javascript
> db.invalidateFsCache()
```

* Clear the file system cache that out of 72 hours of 'group1' nodes.

```lang-javascript
> db.invalidateFsCache( { GroupName: 'group1' }, "72" )
```



[^_^]:
    links
[location]:manul/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
