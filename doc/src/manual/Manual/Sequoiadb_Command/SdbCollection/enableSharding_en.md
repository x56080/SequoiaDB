##NAME##

enableSharding - modify the properties of the collection to turn on the sharding properties

##SYNOPSIS##

**db.collectionspace.collection.enableSharding(\<options\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to modify the properties of the collection to turn on the sharding properties.

##PARAMETERS##

options ( *object, required* )

Modify the collection properties through the parameter "options":

- ShardingKey ( *object, required* ): Sharding key, and the value is 1 or -1, indicating forward or reverse sorting.

    "ShardingKey" can be modified when the collection only exists in one data group, or the collection does not have subcollections mounted.

    Format: `ShardingKey: {<field1>: <1|-1>, [<field2>: <1|-1>, ...]}`

- ShardingType ( *string* ): Sharding type, and the default value is "hash".

    The values are as follows:

    - "hash": Hash sharding.
    - "range": Range sharding.

    "ShardingType" can be modified when the collection only exists in one data group.

    Format: `ShardingType: "range"`

- Partition ( *number* ): The number of sharding, and the default value is 4096.

    - The value of this parameter must be a power of 2, and the range is [2\^3, 2\^20].
    - This parameter can only take effect when the value of the parameter "ShardingType" is "hash".
    - "Partition" can be modified when the collection only exists in one data group.

    Format: `Partition: 512`
   
- AutoSplit ( *boolean* ): Whether to enable the automatic segmentation function, and the default value is "false", which means that automatic segmentation is not enabled.

    - This parameter can only take effect when the value of the parameter "ShardingType" is "hash".
    - "AutoSplit" can be modified when the collection only exists in one data group.

    Format: `AutoSplit: true`

    >**Note:**
    >
    > User can specify the parameter "AutoSplit" when creating domains and collections. If user explicitly specify "AutoSplit" for a collection, the system will give priority to determining whether to enable automatic splitting according to the value specified by the collection.

- EnsureShardingIndex ( *boolean* )：Whether to automatically create an index named "$shard" according to the field specified by the parameter "ShardingKey", and the default value is "true", which means automatically created.

    "EnsureShardingIndex" can be modified when the collection only exists in one data group.

    Format: `EnsureShardingIndex: false`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `enableSharding()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported | Check the attributes of the current collection, if it is a partitioned collection, users cannot modify the attributes related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

- Create a normal collection, and then modify the collection to a sharding collection.

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- Create a normal collection, then modify the collection to a sharding collection, and split it automatically. 

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md