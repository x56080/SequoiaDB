##语法##
***traceFmt( \<formatType\>, \<input\>, \<output\> )***

将 db.traceOff() 导出来的二进制文件格式化输出到指定文件。

##参数描述##

| 参数名    | 参数类型 | 描述     | 是否必填 |
| --------- | -------- | -------- | -------- |
| formatType| int      | 格式类型 | 是       |
| input     | string   | 输入文件 | 是       |
| output    | string   | 输出文件 | 是       |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -189			| 跟踪文件不合法| 确认所用文件是否合法	|

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 格式化输出文件

 ```lang-javascript
> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace.flw" )
 ```