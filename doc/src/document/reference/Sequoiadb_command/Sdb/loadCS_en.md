##NAME##

unloadCS -  Load the specific collection space into memory.

##SYNOPSIS##

***db.loadCS( \<csName\>, [options] )***

##CATEGORY##

Sdb

##DESCRIPTION##

Load the specific collection space into memory.

##PARAMETERS##

| Name    | Type   | Default | Description                          | Required or not |
| ------- | ------ | ------- | ------------------------------------ | --------------- |
| csName  | string | ---     | collection space name                | yes             |
| options | JSON   | NULL    | [command position parameter](reference/Sequoiadb_command/location.md) | not             |

>**Note:**

>Only when connecting to the coordination node, the options parameter will take effect.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Query data. ( Suppose the specific collection space named "foo" existes and the current sequoiadb is started in standalone mode )

```lang-javascript
> db.foo.bar.find()
{
   "_id": {
     "$oid": "5d36c9d5c6b1cee56abefc7e"
   },
   "name": "fang",
   "age": 18
}
```  

* Unload the collection space named foo from memory.

```lang-javascript
> db.unloadCS( "foo" )
```

* Query data.

```lang-javascript
> db.foo.bar.find()
uncaught exception: -34
Collection space does not exist
``` 

* Load the collection space named into memory.

```lang-javascript
> db.loadCS( "foo" )
```

* Query data again.

```lang-javascript
> db.foo.bar.find()
{
   "_id": {
     "$oid": "5d36c9d5c6b1cee56abefc7e"
   },
   "name": "fang",
   "age": 18
}
```  