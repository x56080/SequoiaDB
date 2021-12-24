##NAME##

createIndex - create index

##SYNOPSIS##

**db.collectionspace.collection.createIndex\(\<name\>, \<indexDef\>, \[isUnique\], \[enforced\], \[sortBufferSize\])**

**db.collectionspace.collection.createIndex\(\<name\>, \<indexDef\>, \[indexAttr\], \[option\])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to create an index for the collection to improve query speed.

##PARAMETERS##

- name ( *string, required* )

    Index name. It should be unique in a collection.

- indexDef ( *object, required* )

    Index key. It contains one or more objects that specify index fields and order direction. "1" means ascending order. "-1" means descending order.

- isUnique ( *boolean, optional* )

    Whether the index is unique. The default value is "false". When it is "true", the index is unique.

- enforced ( *boolean, optional* )

    Whether the index is mandatorily unique or not. Its default value is false, and it becomes effective when "isUnique" is true. When it is true, it means that there can be no more than one empty index key.

- sortBufferSize ( *number, optional* )

    The size of sort buffer used when creating index, the unit is MB, zero means don't use sort buffer, the default value is 64.

- indexAttr ( *object, optional* )

    Index attributes can be set through the parameter:
    
    - Unique ( *boolean* ): Whether the index is unique. The defalut value is false.
    
    - Enforced ( *boolean* ): Whether the index is mandatorily unique. The defalut value is false.
    
    - NotNull ( *boolean* ): Whether any field of index is allowed to be null or not-existent. The defalut value is false.
    
    - NotArray ( *boolean* ): Whether any field of index is allowed tobe an array. The defalut value is false.
    
    - Standalone ( *boolean* ): Whether it is a [standalone index][standalone]. The defalut value is false.

- options ( *object, optional* )

    Other optional parameters can be set through the options parameter:

    - SortBufferSize ( *number* ): The size of sort buffer used when creating index. The defalut value is 64 MB.
    
    - NodeName ( *string/array* ): Specify the name of data node when a standalone index is created. The format is \<hostname\>:\<svcname\>. Used in conjunction with the parameter Standalone.
    
   - NodeID ( *number/array* ): Specify the ID of data node when a standalone index is created. Used in conjunction with the parameter Standalone.
    
    - InstanceID ( *number/array* ): Specify the ID of instance when a standalone index is created. Used in conjunction with the parameter Standalone.

> **Note:**
>
> - There should not be any exactly same records in the fields that are specified by the unique index in a collection.
> - Index name should not be null string. It should not contain "." or "$". The length of it should be no more than 127B.
> - When the collection record data volume is large(more than 10 million records), appropriately increasing the sort cache size can increase the speed of index creation.
> - For text index, the parameters isUnique, enforced and sortBufferSize are meaningless.
> - Standalone index can be selectively designated to be created on the primary or secondary data node.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Create an unique index named "ageIndex" on the field "age" in collection sample.employee. The records are in ascending order on the field "age".

    ```lang-javascript
    > db.sample.employee.createIndex( "ageIndex", { age: 1 }, true )
    ```
   
* Create an unique index in collection sample.employee, and any field of index should exist and cannot be null.

    ```lang-javascript
    > db.sample.employee.createIndex( "ab", { a: 1, b: 1 }, { Unique: true, NotNull: true } )
    > 
    > // "b" field is null. Insert will throw error.
    > db.sample.employee.insert( { a: 1, b: null } )
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    > 
    > // "b" field does not exist. Insert will throw error.
    > db.sample.employee.insert( { a: 1 } )
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    ```

* Create a text index named "addr_tag" on field "address" and "tags" in collection "bar", which will be used for full text search.

    ```lang-javascript
    > db.foo.bar.createIndex( "addr_tags", { address: "text", tags: "text" } )
    ```
* Create an unique index in collection sample.employee, and any field of index not support array.

    ```lang-javascript
    > db.sample.employee.createIndex( "ab", { a: 1, b: 1 }, { NotArray: true} )
    >
    > // "a" field is array. Insert will throw error.
    > db.sample.employee.insert( { a: [1],b: 10 } )
    sdb.js:645 uncaught exception: -364
    Any field of index key cannot be array
    ```

* Assuming that the collection sample.employee is in the data group 'group1', and the node `sdbserver:11850` belongs to 'group1', create a standalone index on this node.

    ```lang-javascript
    > db.sample.employee.createIndex( "a", { a: 1 }, { Standalone: true }, { NodeName: "sdbserver:11850" } )
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[standalone]:manual/Distributed_Engine/Architecture/Data_Model/index.md#创建索引
