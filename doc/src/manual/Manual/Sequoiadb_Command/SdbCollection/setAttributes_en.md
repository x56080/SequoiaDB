##NAME##

setAttributes - modify the properties of the collection

##SYNOPSIS##

**db.collectionspace.collection.setAttributes(\<options\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to modify the properties of the collection.

##PARAMETERS##

options ( *object, required* )

Modify the collection properties through the parameter "options":

- ReplSize ( *number* )：The number of replicas that need to be synchronized for write operations, and the default value is 1, which means that write operations only need to be written to the master node.

    The values are as follows:

    - -1: The write request needs to be synchronized to a number of active nodes in the replication group before the database write operation returns a response to the client.
    - 0: The write request needs to be synchronized to all nodes in the replication group before the database write operation returns a response to the client.
    - 1~7: The write request needs to be synchronized to the specified number of nodes in the replication group before the database write operation returns a response to the client.

    Format: `ReplSize: 0`

- ConsistencyStrategy ( *number* ): [Synchronization consistency][consistency_strategy] strategy.

    This parameter is used to set the preferred node for data synchronization, the default value is 3.

    The values are as follows:

    - 1: Node priority strategy.
    - 2: Position majority first strategy.
    - 3: Main position majority first strategy.

    Format：`ConsistencyStrategy: 3`

- ShardingKey ( *object* ): Sharding key, and the value is 1 or -1, indicating forward or reverse sorting.

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

- EnsureShardingIndex ( *boolean* ): Whether to automatically create an index named "$shard" according to the field specified by the parameter "ShardingKey", and the default value is "true", which means automatically created.

    "EnsureShardingIndex" can be modified when the collection only exists in one data group.

    Format: `EnsureShardingIndex: false`

- Compressed ( *boolean* ): Whether to enable the data compression function, and the default value is "true", which means that the data compression function is enable.

    Format: `Compressed: false`

- CompressionType ( *string* ): Compression algorithm type, and the default value is "lzw".

    The values are as follows:

    - "snappy": Snappy algorithm compression.
    - "lzw": Lzw algorithm compression.

    Format: `CompressionType: "snappy"`

    >**Note:**
    >
    > For the usage scenarios of "snappy" compression and "lzw" compression, please refer to [data compression][date_compression].

- StrictDataMode ( *boolean* ): Whether to enable strict data type mode, and the default value is "false", which means it is not enable.

    After enabling strict mode, if the data type is numeric, an error will be reported if an overflow occurs during the operation. If the data type is not numeric, no operation will be performed.

    Format: `StrictDataMode: true`

- AutoIncrement ( *object* )：Properties of auto-incrementing field.

    - The properties that are allowed to be modified can refer to the [auto-increment field][sequence].
    - After modifying the attribute, the field value may not be unique. If users need to ensure that the modified value is unique, it is recommended to use a unique index.

    Format: `AutoIncrement: {Field: <Field name>, ...}` or `AutoIncrement: [{Field: <Field name1>, ...}, {Field: <Field name2>, ...}, ...]`

- AutoIndexId ( *boolean* ): Whether to automatically create a unique index named "$id" based on the field "_id", and the default value is "true", which means automatically created.

    Format: `AutoIndexId: false`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setAttributes()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported. | Check the attributes of the current collection, if it is a partitioned collection, user cannot modify the attributes related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##示例##

##EXAMPLES##

- Create a normal collection, and then modify the collection to a partitioned collection.

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.setAttributes({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- Create a normal collection, then modify the collection to a partitioned collection, and split it automatically.

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.setAttributes({ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```

- Create a normal collection, and then modify the collection to "snappy" compression.

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.setAttributes({CompressionType: "snappy"})
    ```

- Create a collection with auto-increment fields and modify its auto-increment starting value.

    ```lang-javascript
    > db.sample.createCL("employee", {AutoIncrement: {Field: "studentID"}})
    > db.sample.employee.setAttributes({AutoIncrement: {Field: "studentID", StartValue: 2017140000}})
    ```


[^_^]:
    本文使用的所有引用及链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[date_compression]:manual/Distributed_Engine/Architecture/compression_encryption.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md