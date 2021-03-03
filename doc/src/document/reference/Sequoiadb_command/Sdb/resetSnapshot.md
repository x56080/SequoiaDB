##语法##
***db.resetSnapshot( [options] )***

重置快照。

##参数描述##

| 参数名  | 参数类型 | 描述   | 是否必填 |
| ------- | -------- | ------ | -------- |
| options | Json 对象| 设定[命令位置参数](reference/Sequoiadb_command/location.md) | 否 |

1. **options 格式**

 | 属性名 | 描述   | 默认值 | 格式 |
 | ------ | ------ | -------| ---- |
 | Location Elements | [命令位置参数](reference/Sequoiadb_command/location.md) | 所有节点 | GroupName:"db1" |

##返回值##
无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。

关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##示例##

* 重置快照。

  ```lang-javascript
  > db.resetSnapshot()
  ```