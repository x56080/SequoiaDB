##语法##

***System.getCurrentUser()***

##类别##

System

##描述##

获取当前用户信息

##参数##

无

##返回值##

返回当前用户信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 获取当前用户信息

  ```lang-javascript
  > System.getCurrentUser()
  {
      "user": "root",
      "gid": "0",
      "dir": "/root"
  }
  ```