## 名称

getServiceName - 获取节点的服务器名

## 语法

**node.getServiceName()**

## 类别

SdbNode

## 描述

获取节点的服务器名。

## 参数

无

## 返回值

返回节点的服务器名，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

## 错误

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 版本

v2.0 及以上版本。

## 示例

* 获取 node 节点的服务器名

 ```lang-javascript
 > node.getServiceName()
 11800
 ```
