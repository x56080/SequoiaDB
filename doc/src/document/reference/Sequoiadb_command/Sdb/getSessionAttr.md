##语法##

***db.getSessionAttr()***

获取会话属性

##返回值##

函数执行成功时，将返回表示会话属性的 Json 对象，返回值字段信息可参考 [Sdb.setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md)。

函数执行失败时，将抛异常并输出错误信息。可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码，关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

> **Note:**
>
>* 用户在独立模式或者非协调节点上执行该函数时，将返回空的 Json 对象。
>* 如果当前会话属性不符合预期，可使用 [Sdb.setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md) 设置会话属性。

##示例##

* 获取会话属性

 ```lang-javascript
 > db.getSessionAttr()
 {
  "PreferedInstance": "M",
  "PreferedInstanceMode": "random",
  "Timeout": -1
 }
 ```
