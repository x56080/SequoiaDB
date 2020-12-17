##语法##
***node.getNodeDetail()***

获取当前节点信息。

##参数描述##

无

##返回值##

返回当前节点信息（NodeID:HostName:ServiceName(GroupName)），出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 获取 node 节点的信息

 ```lang-javascript
 > node.getNodeDetail()
  1000:vmsvr2-suse-x64:11800(group)
 ```
