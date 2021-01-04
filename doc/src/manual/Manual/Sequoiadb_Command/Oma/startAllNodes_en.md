
##NAME##

startAllNodes - Start all nodes with the specified business name at the target host.

##SYNOPSIS##

**startAllNodes(\[businessName\])**

##CATEGORY##

Oma

##DESCRIPTION##

Start all nodes with the specified business name at the target host.

**Note:**

* Oma object is a connect object 
* If no business name is specified,all nodes at the target host will be started by default.

##PARAMETERS##

* businessName( *String*， *Optional* )

   The business name of the nodes to start.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##
When exception happen, use getLastError() to get the error code and use getLastErrMsg() to get error message.  For more detial, please reference to Troubleshooting.

##HISTORY##

Since v2.8

##EXAMPLES##

1. Start all nodes with business name "yyy" locally

 	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.startAllNodes( "yyy" )
    Start sequoiadb(20000): Success
    Start sequoiadb(40000): Success
    Start sequoiadb(30020): Success
    Start sequoiadb(50000): Success
    Start sequoiadb(30010): Success
    Start sequoiadb(30000): Success
    Start sequoiadb(42000): Success
    Start sequoiadb(41000): Success
    Total: 8; Success: 8; Failed: 0
 	```