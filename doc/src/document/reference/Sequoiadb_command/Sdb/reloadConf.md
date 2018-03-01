##语法##
***db.reloadConf( [options] )***

重新加载配置文件，并进行配置动态生效。如果配置项不允许动态生效会被忽略。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | Json对象 | **[命令位置参数](reference/Sequoiadb_command/Overview/location.md)** | 否 |

> **Note:**
>
> * 配置项不允许动态生效会被忽略。
> * 命令位置参数填 {Global:true} 时，对除协调节点外的所有节点生效。填 {Global:false} 时则只对协调节点生效。
> * 无命令位置参数时，缺省使用 {Global:true} 的命令位置参数。


##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。
更多错误可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##版本信息##
2.8及以上版本

##示例##

* 对（除了协调节点之外的）所有节点进行配置重新加载。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.reloadConf()
 ```

* 对协调节点进行配置重新加载。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.reloadConf( {Global:false} )
 ```

* 对指定节点 1000 进行配置重加载。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.reloadConf( {NodeID:1000} )
 ```
