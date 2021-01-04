
##NAME##

stopAllNodes - Stop all nodes with the specified business name at the target host.

##SYNOPSIS##

**stopAllNodes (\[businessName\])**

##CATEGORY##

Oma

##DESCRIPTION##

Stop all nodes with the specified business name at the target host.

**Note:**

* Oma object is a connect object 
* If no business name is specified,all nodes at the target host will be stopped by default.

##PARAMETERS##

* businessName( *String*， *Optional* )

   The business name of the nodes to stop.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##
When exception happen, use getLastError() to get the error code and use getLastErrMsg() to get error message.  For more detial, please reference to Troubleshooting.

##HISTORY##

Since v2.8

##EXAMPLES##

1. Stop all nodes with business name "yyy" locally

 	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.stopAllNodes( "yyy" )
    Stop sequoiadb(30000): Success
    Stop sequoiadb(30010): Success
    Stop sequoiadb(30020): Success
    Stop sequoiadb(20000): Success
    Stop sequoiadb(40000): Success
    Stop sequoiadb(41000): Success
    Stop sequoiadb(42000): Success
    Stop sequoiadb(50000): Success
    Total: 8; Success: 8; Failed: 0
 	```