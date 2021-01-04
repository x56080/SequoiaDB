
##NAME##

createIndex - Create an index for the collection to accelerate query.

##SYNOPSIS##

**db.collectionspace.collection.createIndex( \<name\>, \<indexDef\>, [isUnique], [enfored], [sortBufferSize] )**

**db.collectionspace.collection.createIndex( \<name\>, \<indexDef\>, [options] )**

##CATEGORY##

Collection

##DESCRIPTION##

Create an index for the collection to accelerate query.

##PARAMETERS##

* `name` ( *String*, *Required* )

	Index name. It should be unique in a collection.

* `indexDef` ( *Json Object*, *Required* )

	Index key. It contains one or more objects that specify index fields and order direction. "1" means ascending order. "-1" means descending order.

* `isUnique` ( *Boolean*, *Optional* )

	Whether the index is unique. The default value is "false". When it is "true", the index is unique.

* `enforced` ( *Boolean*, *Optional* )

	Whether the index is mandatorily unique or not. Its default value is false, and it becomes effective when "isUnique" is true. When it is true, it means that there can be no more than one empty index key.

* `sortBufferSize` ( *Int*, *Optional* )

	The size of sort buffer used when creating index, the unit is MB, zero means don't use sort buffer, the default value is 64.

* `options` ( *Json Object*, *Optional* )

	Options for creating index. Can be the follow options:

	* Unique (Boolean): Whether the index is unique.
	* Enforced (Boolean): Whether the index is enforced unique.
	* NotNull (Boolean): Whether any filed of index can be null or not exist.
	* SortBufferSize (Int): The size of sort buffer used when creating index.

**Note:**

1. There should not be any exactly same records in the fields that are specified by the unique index in a collection.
2. Index name should not be null string. It should not contain "." or "$". The length of it should be no more than 127B.
3. IndexPageSize 4096 / 8192 / 16384 / 32768 / 65536 bytes, the maximum index value is 1024 / 2048 / 4096 / 4096 / 4096 bytes. 
4. The number of the index fields should be no more than 32.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##EXAMPLES##

1. Create an unique index named "ageIndex" on the field "age" in collection "employee". The records are in ascending order on the field "age".

	```lang-javascript
	> db.sample.employee.createIndex( "ageIndex", { age: 1 }, true )
	```
   
2. Create an unique index in collection "employee", and any field of index should exist and cannot be null.

	```lang-javascript
	> db.sample.employee.createIndex( "ab", { a: 1, b: 1 }, { Unique: true, NotNull: true } )
	> 
	> // "b" field is null. Insert will throw error.
	> db.sample.employee.insert( { a: 1, b: null } )
	sdb.js:625 uncaught exception: -339
	Any field of index key should exist and cannot be null
	Takes 0.002531s.
	> 
	> // "b" field does not exist. Insert will throw error.
	> db.sample.employee.insert( { a: 1 } )
	sdb.js:625 uncaught exception: -339
	Any field of index key should exist and cannot be null
	Takes 0.002531s
	```

3. Create a text index named "addr_tag" on field "address" and "tags" in collection "bar", which will be used for full text search.

	```lang-javascript
	 > db.foo.bar.createIndex( "addr_tags", { address: "text", tags: "text" } )
	 ```