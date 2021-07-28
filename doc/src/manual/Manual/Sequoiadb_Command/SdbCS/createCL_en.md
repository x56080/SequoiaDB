##NAME##

createCL - Create a new collection.

##SYNOPSIS##
**db.collectionspace.createCL(\<name\>,[options])**

##CATEGORY##
Collection Space

##DESCRIPTION##
Create a collection in a specified collection space.
Collection is a logical object which stores records. Each record should 
belong to one and only one collection.

##PARAMETERS##

* `name` ( *String* , *Required* )

    The name of the collection, should be unique to each other in 
    a collection space.

* `options` ( *Object*, *Optional* )

    The options for creating the collection, could be a combination of 
    the following options:

    1. `ShardingKey` ( *Object* ): Sharding key, the value is 1 or -1, indicating forward or backward sorting.
    
        Format: `ShardingKey:{<field1> : <1|-1>,[<field1> : <1|-1>, ...]}`

    2. `ShardingType` ( *String* ): The sharding type of the collection, 
                                    could be one of the following:
        * "hash": Sharding by hash.
        * "range": Sharding by range.
      
        Format: `ShardingType:"hash"|"range"`

    3. `Partition` ( *Int32* ): Specify the number of hash slices, only valid when 
                                 `ShardingType` is "hash". The number of hash slices 
                                 should be power of 2 and in the range of [2\^3，2\^20],
                                 default to be 1024.
                                 
        Format: `Partition: <num>`

    4. `ReplSize` ( *Int32* ): Number of WriteConcern, default to be 1,
                                can be one of the follow:

        * -1: WriteConcern equals to the number of the active nodes in the replica group.
        * 0: WriteConcern equals to the number of all the nodes in replica group.
        * 1 - 7: WriteConcern = 1-7.
  
        Format: `ReplSize: <num>`
  
    5. `Compressed` ( *Bool* ): Compress the collection or not, default to be true.
  
        Format: `Compressed:true|false`
      
    6. `CompressionType` ( *String* ): The algorithm for compressing, 
                                       default to be "lzw", can be one of the follow:

        * "snappy": use snappy algorithm to compress.
        * "lzw": use lzw algorithm to compress.
      
        Format: `CompressionType:"snappy"|"lzw"`
      
    7. `IsMainCL` ( *Bool* ): Is main collection or not, default to be false.
  
        Format: `IsMainCL:true|false`
      
    8. `AutoSplit` ( *Bool* ): Automatic split or not, default to be false.
  
        Format: `AutoSplit:true|false`
      
    9. `Group` ( *String* ): Specify a replica group which the 
                             collection should be in.
  
        Format: `Group:<group name>`
  
    10. `AutoIndexId` ( *Bool* ): Creating unique index by using field "_id" 
                                  or not, default to be true.
   
        Format: `AutoIndexId:true|false`
  
    11. `EnsureShardingIndex` ( *Bool* ): Creating index by using the fields
                                          of the `ShardingKey` or not, default 
                                          to be true.
                                             
        Format: `EnsureShardingIndex:true|false`

    12. `StrictDataMode` ( *Bool* ): using strict date mode in numeric operations 
or not, default to be false.

        Format: `StrictDataMode:true|false`
        
    13. `AutoIncrement` ( *Object* )：Specify auto increment field

        Format: `AutoIncrement:{Field: <field1>, ...}` or `AutoIncrement:[ {Field: <field1>, ...}, {Field: <field2>, ...}, ... ]`

        Example: `AutoIncrement: { Field: "userID", Generated: "always" }`

        * See [AutoIncrement](manual/Distributed_Engine/Architecture/Data_Model/sequence.md) for more detail.
        
    14. `LobShardingKeyFormat` ( *String* )：Specify Lob ShardingKey's value format on main collection. Can be one of the follow:
    
        * "YYYYMMDD": Transform a lob's ID to date string format, for example: "20190701".
        * "YYYYMM": Transform a lob's ID to month string format, for example: "201907".
        * "YYYY"：Transform a lob's ID to year string format, for example: "2019".
    
        Format: `LobShardingKeyFormat:"YYYYMMDD"|"YYYYMM"|"YYYY"`

> **Note:**
> 
> * The parameter `name` can not be an empty string, and can not 
    include "." or "$"; The length of it should not be greater 
    than 127B, or exception will occur.
> 
> * The parameter `options` haves one or more fields, please use
    comma(,) to separate.
> 
> * When creating collection space, user can specify `Domain`. When creating collection, using `Group` parameter, the specified replication group must be in the domain; when no using `Group` parameter, the collection will be created on any replication group in the domain.
>     
> * `AutoSplit` parameter in createCL() has higher priority than `AutoSplit` attribute in domain.
>     
> * `AutoSplit` must cooperate with hash partitioning.
> 
> * `AutoSplit` and Group cannot be set at the same time.
> 
> * Compression algorithm selection strategy: the snappy algorithm compresses data in units of a single record, and the internal data repeatability directly affects the compression ratio. Therefore, when the internal data of the record is relatively high in repetition, such as the field name and field value of a record are similar, the snappy algorithm can be used to obtain good compression performance. If the internal data of record is very low in repetition, but the records have higher similarity, such as different records with the same field name, similar field values, etc., lzw compression is better.
> 
> * `LobShardingKeyFormat` can only be used in main collection and requires that the ShardingKey have only one partition field.
> 
> * When using main collection and sub collection, you need to pay attention to the use relationship of their attributes:
>     1. When inserting data from main collection, `ReplSize`, `AutoIncrement` will use main collection attributes value.
>     2. When inserting data from sub collection, `ReplSize`, `AutoIncrement` will use sub collection attributes value.
>     3. For other attributes of the collection, such as `ShardingKey`, `Compressed`, `AutoIndexId` and so on, sub collection uses its own attributes instead of the attributes of main collection.

##RETURN VALUE##

On success, createCL() creates a new collection and return the object.

On error, exception will be thrown.

##ERRORS##

The exceptions of `createCL()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -2 | SDB_OOM | Out of Memory. | Check the settings and usage of physical memory and virtual memory. |
| -11 | SDB_NOSPC | Out of space. | Check the remaining disk space. |
| -22 | SDB_DMS_EXIST | Collection already exist. | Check whether the collection exist or not. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more detail.

##HISTORY##
Since v1.0.

##EXAMPLES##
1. Create collection "bar" in collection space "foo" without sharding key.


	```lang-bash
    > db.foo.createCL( "bar" )
    localhost:11810.foo.bar
    Takes 0.1250s.
	```

2. Create collection "bar" in collection space "foo". If the collection splits data into other replication groups, it will use the age field for hash segmentation. It has data compression enabled by default, using the default lzw algorithm.When a write operation is applied to the collection, it only needs to be written to the primary node to be returned.

    ```lang-bash
    > db.foo.createCL( "bar", { ShardingKey:{ age: 1 }, ShardingType: "hash", 
                                Partition: 4096, ReplSize: 1 } )
    localhost:11810.foo.bar
    Takes 0.32450s.
    ```
3. Create collection "bar" in collection space "foo" with the StrictDataMode turn on.

    ```lang-bash
    > db.foo.createCL( "bar", { StrictDataMode: true } )
    localhost:11810.foo.bar
    Takes 0.1250s.
    ```

4. Deal with main collection with lob.
    * Create a main collection "maincl" in collection space "foo" with LobShardingKeyFormat "YYYYMMDD".

    ```lang-bash
    > db.foo.createCL("maincl", { LobShardingKeyFormat:"YYYYMMDD", ShardingKey:{ date:1 }, IsMainCL:true, ShardingType:"range" } )
    localhost:11810.foo.maincl
    Takes 0.058532s.
    > db.foo.createCL("subcl")
    localhost:11810.foo.subcl
    Takes 0.294612s.
    > db.foo.maincl.attachCL( "foo.subcl", { LowBound: { date: "20190701" }, UpBound: { date: "20190801" } } )
    Takes 0.008561s.
    ```

    * Time between[20190701, 20190801), Lob which is putted into collection "maincl" will be actually stored in foo.subcl

    ```lang-bash
    > Timestamp()
    Timestamp("2019-07-23-18.04.07.539050")
    > db.foo.maincl.putLob('/opt/data/test.dat')
    00005d36dbee370002de8080
    Takes 0.246062s.
    ```

    * Time can be specified to a particular time.

    ```lang-bash
    > db.foo.maincl.createLobID("2019-07-23-18.04.07.539050")
    00005d36db97360002de8081
    Takes 0.108365s.
    > db.foo.maincl.putLob('/opt/data/test.dat', '00005d36db97360002de8081')
    00005d36db97360002de8081
    Takes 0.002216s.
    ```
