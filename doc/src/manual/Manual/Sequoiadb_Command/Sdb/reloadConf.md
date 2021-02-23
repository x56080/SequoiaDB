##名称##

reloadConf - 重新加载配置文件

##语法##

**db.reloadConf( [options] )**

##类别##

Sdb

##描述##

该函数用于重新加载配置文件，并进行配置动态生效。如果配置项不允许动态生效会被忽略。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

> **Note:**
>
> * 配置项不允许动态生效会被忽略。
> * 无命令位置参数时，缺省对所有节点生效，即使用 {Global:true} 的命令位置参数。


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.8 及以上版本

##示例##

* 对所有节点进行配置重新加载。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.reloadConf()
 ```

* 对指定节点 1000 进行配置重加载。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.reloadConf( {NodeID:1000} )
 ```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md