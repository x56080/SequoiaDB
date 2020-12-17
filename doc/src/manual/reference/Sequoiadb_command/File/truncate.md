##语法##

***File.truncate( \[size\] )***

##类别##

File

##描述##

将文件截断到给定的长度。如果 size 大于文件，则文件扩展为空字节；如果 size 小于文件，则文件将被截断为该大小。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述                 | 是否必填 |
| ------ | -------- | ------ | -------------------- | -------- |
| size   | int      | 0      | 文件截断为 size 字节 | 否       |

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。  

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 截断一个文件；

  ```lang-javascript
  > var file = new File( "/opt/sequoiadb/file.txt" )
  > file.truncate()
  ```