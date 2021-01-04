
##语法##
***db.setPDLevel( \<level\>, [options] )***

动态设置节点的诊断日志级别。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| level  | int    | 日志级别，取值 0~5，分别对应<br> 0: SEVERE<br> 1: ERROR<br> 2: EVENT<br> 3: WARNING<br> 4: INFO<br> 5: DEBUG   | 是 |
| options | Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

> **Note:**
>
> * 日志级别不正确时默认设置为 WARNING。
> * 无位置参数时，缺省只对本身节点生效。
> * 该设置参数不会被持久化。


##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md)  或 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
更多错误可以参考[常见错误处理指南](manual/faq.md) 。

##版本信息##
2.8及以上版本

##示例##

* 设置当前节点的日志级别为DEBUG。

 ```lang-javascript
 // 连接节点
 > db = new Sdb( "localhost", 11810 )
 > db.setPDLevel( 5 )
 ```

* 设置节点1000的日志级别为INFO。

 ```lang-javascript
 // 连接节点
 > db = new Sdb( "localhost", 11810 )
 > db.setPDLevel( 4, {NodeID:1000} )
 ```

* 设置所有节点的日志级别为EVENT。

 ```lang-javascript
 // 连接节点
 > db = new Sdb( "localhost", 11810 )
 > db.setPDLevel( 3, {Global:true} )
 ```
