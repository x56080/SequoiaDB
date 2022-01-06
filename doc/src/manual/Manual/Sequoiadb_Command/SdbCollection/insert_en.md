
##NAME##

insert - Insert record or records into the current collection.

##SYNOPSIS##

**db.collectionspace.collection.insert(\<doc|docs\>,[flag])**

**db.collectionspace.collection.insert(\<doc|docs\>,[options])**

##CATEGORY##

Collection

##DESCRIPTION##

Insert record or records into the current collection. If the current collection does not exist, please create it first.

##PARAMETERS##

* `doc/docs` ( *Object/Array of Object*， *Required* )

	Record or records to be inserted. Can not be null.

* `flag` ( *Int32*， *Optional* )

	Insert flag, use to control the behavior and result of inserting. Can be the follow value:

	* 0, default value.
	* SDB_INSERT_RETURN_ID, means return the value of "_id" field of the record.
	* SDB_INSERT_CONTONDUP, means continue inserting (no errors were reported) when hitting index key duplicate error.
    * SDB_INSERT_REPLACEONDUP, means replace the exist value to the inserting value when hitting index key duplicate error, and continue the left records (no errors were reported).

* `options` ( *Object*， *Optional* )

	Insert options, use to control the behavior and result of inserting. So far, its role is consistent with the argument `flag`, and it can be the follow value:
	* `ReturnOID` ( *Bool*, *Optional* ): the same with "SDB_INSERT_RETURN_ID" in argument `flag`.
	* `ContOnDup` ( *Bool*, *Optional* ): the same with "SDB_INSERT_CONTONDUP" in argument `flag`.
	* `ReplaceOnDup` ( *Bool*, *Optional* ): the same with "SDB_INSERT_REPLACEONDUP" in argument `flag`.

**Note:**

* When the provided record does not have an "_id" field, the database will add one for it.

* In argument `flag`, SDB_INSERT_CONTONDUP and SDB_INSERT_REPLACEONDUP can not be use at the same time. In argument `options`, that is the same.

##RETURN VALUE##

* On success, the follow result will be returned:

 ```
 {
		InsertedNum    : <INT32>  Number of records successfully inserted, including replaced and ignored records,
		DuplicatedNum  : <INT32>  Number of records ignored or replaced due to duplicate key conflicts
 }
 ```

  When using "SDB_INSERT_RETURN_ID" in flag or "ReturnOID" in options, the result also include field "_id", as follows:
	* for single inserting: return the value of field "_id".
	* for bulk inseting: return the value of field "_id" by array.


On error, exception will be thrown.

##ERRORS##

the exceptions of `insert()` are as below:

| Error code | Error type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | Invalid Argument. | Check whether the argument is invalid or not. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist. | heck whether the collection exist or not. |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist. | Check whether the collection space exist or not. |
| -38 | SDB_IXM_DUP_KEY | The same unique index value conflicts with this record. | Check the record to make sure no duplicate  index key is offered. |

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Insert a record.

	```lang-javascript
 	> db.sample.employee.insert( { name: "Tom", age: 20 } )
 	```

2. Insert some records. 

 	```lang-javascript
 	> db.sample.employee.insert( [ { _id: 20, name: "Mike", age: 15 }, { name: "John", age: 25, phone: 123 } ] )
 	```

3. Insert records with duplicate index keys.

	```lang-javascript
 	> db.sample.employee.insert( [ { _id: 1, a: 1 }, { _id: 1, b:2 }, { _id: 3, c: 3 } ],  SDB_INSERT_CONTONDUP )
 	```

 	```lang-javascript
 	> db.sample.employee.find()
 	{
      	"_id": 1,
      	"a": 1,
 	}
 	{
      	"_id": 3,
      	"c": 3
 	}
 	```

4. Insert record, and return Json object.

	```lang-javascript
 	> db.sample.employee.insert({a:1}, {ReturnOID:true, ContOnDup:true})
 	{
   		"_id": {
     		"$oid": "5becec3d6404b9295a63caca"
   		}
		"InsertedNum": 1,
  		"DuplicatedNum": 0
 	}

	```

	
	```lang-javascript
 	> db.sample.employee.insert([{a:1}, {b:1}], {ReturnOID:true, ContOnDup:true})
 	{
   		"_id": [
     		{
       			"$oid": "5bececdf6404b9295a63cacb"
     		},
     		{
       			"$oid": "5bececdf6404b9295a63cacc"
     		}
   		]
		"InsertedNum": 2,
		"DuplicatedNum": 0
 	}
 	```