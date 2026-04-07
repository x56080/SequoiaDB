##NAME##

diaglog - Create a new DiagLog object

##SYNOPSIS##

**var diaglog = new DiagLog()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Create a new DiagLog object.

##PARAMETERS##

NULL

##RETURN VALUE##

NULL

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

1. Create a DiagLog object.

	```lang-javascript
 	> var diaglog = new DiagLog()
 	```

>**Note:**
>
> When users user the `DiagLog()` for searching, specifying additional conditions will affect the search speed. 
>
> When specifying the following search conditions, the search method can be optiomzied to speed up the search process:
>
> - lastFile() Set search() to only search recent log files  
> - error() Set the error code for the search() function  
> - diaglevel() Set the log level to filter in search()  
> - keypattern() Set the keywords to search in search()  
> - pid() Set the pid for the search() function  
> - tid() Set the tid for the search() function  
> - limit() Limit the number of results returned by search()  
> - original() Set the search() function to return the raw log format  
>
> When specifying the following search conditions, the search method will not be able to use any optimizations beyond the limit(), and the search speed will be very sloe:
>
> - after() Set the number of items below the search() result context  
> - before() Set the number of items preceding the search() result context  
