
##NAME##

startNodes - Start one or more nodes in target host of sdbcm.

##SYNOPSIS##

**oma.startNodes(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Start one or more nodes in target host of sdbcm.

**Note:**

* Oma object is a connect object 

##PARAMETERS##

* `svcname` ( *String | Int | Array*, *Required* )

   The service name of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `startNodes()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -264 | SDB_COORD_NOT_ALL_DONE| One or more nodes did not complete successfully | Use getLastErrObj() to see which node is failed
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist | Check the node's configuration file |
| -6   | SDB_INVALIDARG | Invalid Argument | Check the value of the svcname parameter |

##HISTORY##

Since v3.0.2.

##EXAMPLES##

1. Start a node with the service name 11810.

 	```lang-javascript
	> var oma = new Oma()
	> oma.startNodes( 11810 )
 	```

2. Start a node with the service name 11820.

 	```lang-javascript
	> var oma = new Oma()
	> oma.startNodes( "11820" )
    ```

3. The nodes with the service names 11810, 11820, and 11830 are concurrently started.

 	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.startNodes( [ 11810, 11820, 11830 ] )
 	```

4. View error message

 	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.startNodes( [ 11810, 11820, 20000 ] )
    (nofile):0 uncaught exception: -264
    One or more nodes did not complete successfully
    > getLastErrObj()
    {
      "errno": -264,
      "description": "One or more nodes did not complete successfully",
      "detail": "",
      "ErrNodes": [
        {
          "errno": -146,
          "description": "Node does not exist",
          "detail": "Failed to start node: svcname=20000",
          "svcname": "20000"
        }
      ]
    }
 	```