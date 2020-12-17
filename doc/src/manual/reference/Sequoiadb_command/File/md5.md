##语法##

***File.md5( \<filepath\> )***

##类别##

File

##描述##

获取文件的 md5 值。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述     | 是否必填 |
| -------- | -------- | ------ | -------- | -------- |
| filepath | string   | ---    | 文件路径 | 是       |

##返回值##

返回指定文件的 md5 值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 获取文件的 md5 值。

  ```lang-javascript
  > File.md5( "/opt/sequoiadb/file.txt" )
  f8fef4e0f30176c126d85cadca298a7c
  ```
