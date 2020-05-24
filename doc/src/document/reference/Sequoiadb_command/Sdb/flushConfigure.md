##语法##
***db.flushConfigure( \[options\] )***

将节点内存中的配置刷盘至配置文件

##参数描述##

|参数名    |参数类型    |描述         |是否必填|
|--------- |----------- |------------ |----------|
| options  | Json对象   | 设定配置过滤类型 以及 **[命令位置参数](reference/Sequoiadb_command/location.md)** | 否 |

1. **options 格式**

 | 属性名 | 描述 | 格式 |
 | ------ | ------ | ------ |
 | Type   | 配置过滤类型， 0:所有配置; 1:屏敝未修改的隐藏参数; 2:屏敝未修改的所有参数; 3:屏敝未配置的参数。缺省取值为 3。 | Type:3 |
 | Location Elements | 命令位置参数项，详细见 **[命令位置参数](reference/Sequoiadb_command/location.md)** | GroupName:"db1" |

> **Note:**
>
> * 配置过滤类型不正确时默认设置为 3。
> * 无位置参数时，缺省只对本身节点生效。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

- 刷盘数据库配置

 ```lang-javascript
 > db.flushConfigure( { Global : true } )
 ```
