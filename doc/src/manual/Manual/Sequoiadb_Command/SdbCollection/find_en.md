
##NAME##

find - Select records from database.

##SYNOPSIS##

**db.collectionspace.collection.find([cond],[sel])**

**db.collectionspace.collection.find([cond],[sel]).hint([hint])**

**db.collectionspace.collection.find([cond],[sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.find([cond],[sel])[.hint([hint])][.skip([skipNum])][.limit([retNum])][.sort([sort])]**

##CATEGORY##

Collection

##DESCRIPTION##

Select records from database. Return a cursor on the selected records. 
Cursor is a pointer in SequoiaDB, it points to a query result set, the 
search result can be iterated in the client.

##PARAMETERS##

* `cond` ( *Object*， *Optional* )

	Selecting condtion. If it is null, it will find all the records. 
    If it is not null, it will find records that matches the condition.

* `sel` ( *Object*， *Optional* )

	It chooses fields to be returned. If it is null, it will return all the fields.
    If a field doesn't exist, it will return the same contents which was input. 
	
	Format: {"filed1":"", "filed2":"", "filed3":""}

* `hint` ( *Object*， *Optional* )

	specified the hint for query.
	* when not specified 'hint', it is up to the database to decide whether 
      to use the index and which index to be used. 
	* when 'hint' is {"":null}, table scan.
	* when 'hint' contains only one index, such as: {"":"myIdx"}, the query will be
      made using the index named "myIdx" in the currrent collection; However, when index "myIdx" does not exist in current collection, query goes with table scan.
	* when 'hint' contains several indexes, such as: {"1":"idx1","2":"idx2","3":"idx3"},
		 	  the query	will be made using one of the three indexes described above. Which 
	  index is used eventually, determined by the database evaluation.

* `skipNum` ( *Int32*， *Optional* )

	specified where returns from the record of the result set. The default value is 0, 
	means returns from the first record of the result set.

* `retNum` ( *Int32*， *Optional* )

	specified how many records to be return from the result set. The default value is -1,
	means returns all the recoreds since the position of "skipNum".

* `sort` ( *Object*， *Optional* )

	Specifies whether the result set is sorted by the specified field name. The 
  	value of the field name is 1 or -1, such as: {"name":1,"age":-1}.
	* when not specified 'sort', means the result set is not sorted.
	* when the value of field name is 1, means sort by field name in ascending order.
	* when the value of field name is -1, means sort by field name in decending order.

* `SdbQueryOption` ( *Object*， *Optional* )

	Use an object to specify record query parameters.For more detial, please  reference to [SdbQueryOption](manual/Manual/Sequoiadb_command/AuxiliaryObjects/SdbQueryOption.md).

**Note：**

* The parameter 'sel' is an object, the values of it's fields are empty string, 
  database only care about the name of the fields.

* The parameter 'hint' is an object, the name of it's fields can be any unique string,
  database onlu care about the value of the fileds.

##RETURN VALUE##

On success, find() returns a object of DBCursor for iterating the result.

On error, exception will be thrown.

##ERRORS##

the exceptions of `find()` are as below:

| Error code | Error type | Description | Solution |
| ------ | ------ | --- | ------ |
| -2 | SDB_OOM | Out of Memory. | Check the settings and usage of physical memory and virtual memory. |
| -6 | SDB_INVALIDARG | Invalid Argument. | Check whether the input arguments are valid or not. |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist. | Check whether the collection space exist or not. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist. | heck whether the collection exist or not. |

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##HISTORY##

Since v1.0.

##EXAMPLES##

1. query all the records without specified condition and selector.

	```lang-javascript
	> db.sample.employee.find()
	```

2. query records with the specified condition. Return the records which name
   is "Tom" and "age" is greater then 25.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 25 }, name: "Tom" } )
	```
3. specified the fields to be returned. when we have recored { age: 25, type: "system" }
   and { age: 20, name: "Tom", type: "normal" }, the follow operation return the contents 
   of field "age" and field "name".

	```lang-javascript
	db.sample.employee.find( null, { age: "", name: "" } )
	 	{
	    	"age": 25,
	      	"name": ""
	 	}
	 	{
	      	"age": 20,
	      	"name": "Tom"
	 	}
	```
4. specifed which index to be use to query.

	```lang-javascript
   > db.sample.test.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
	{
    		"_id": {
    		"$oid": "5812feb6c842af52b6000007"
    		},
    		"age": 10
	}
	{
    		"_id": {
    		"$oid": "5812feb6c842af52b6000008"
    		},
    		"age": 20
	}
	```

5. select records from collection 'sample.employee' and return records which field "age" is greater 
   than 10 by using [$gt](manual/Manual/Operator/match_operator/gt.md). when get the result set,
   we skip the first 3 records and return 5 records.

	```lang-javascript
	> db.sample.employee.find( { age: { $gt: 10 } } ).skip(3).limit(5)
	```
	when the number of records in the result set is not greater that 3, no record is returned.

	when the number of records in the result set is greater that 3, returns up to 5 records.

6. select records from collection 'sample.employee' and return records which filed 'age' is greater
   than 20. we only want filed 'age' and field 'name' to be return. The returned records are
   sort by field 'age' in ascending order.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 }
	```

 'sort' can only apply to the returned fields.

7. specified a useless field name to sort.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
	```

 for the return records do not contain field "sex", so the sort request does not work.

8. Find records which contain "rock climbing" in collection "employee" by using full text search

 ```lang-javascript
> db.sample.employee.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"}}}}})
 ```