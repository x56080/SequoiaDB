##名称##

flushConfigure - 将节点内存中的配置刷盘至配置文件

##语法##

**db.flushConfigure( \[options\] )**

##类别##

Sdb

##描述##

该函数用于将节点内存中的配置刷盘至配置文件。

##参数##

|参数名    |参数类型    |描述         |是否必填|
|--------- |----------- |------------ |----------|
| options  | Json对象   | 设定配置过滤类型 以及 **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

1. **options 格式**

 | 属性名 | 描述 | 格式 |
 | ------ | ------ | ------ |
 | Type   | 配置过滤类型， 0:所有配置; 1:屏敝未修改的隐藏参数; 2:屏敝未修改的所有参数; 3:屏敝未配置的参数。缺省取值为 3。 | Type:3 |
 | Location Elements | 命令位置参数项，详细见 **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | GroupName:"db1" |

> **Note:**
>
> * 配置过滤类型不正确时默认设置为 3。
> * 无位置参数时，缺省只对本身节点生效。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

- 刷盘数据库配置

 ```lang-javascript
 > db.flushConfigure( { Global : true } )
 ```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md