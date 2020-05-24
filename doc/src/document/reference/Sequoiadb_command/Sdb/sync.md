##语法##
***db.sync( [options] )***

持久化数据和日志到磁盘。

##参数描述##

| 参数名 |参数类型| 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options |Json 对象| 设定 **深度刷盘**、 **阻塞模式**、 **指定集合空间** 以及 **[命令位置参数](reference/Sequoiadb_command/location.md)** | 否 |

1. **Options 格式**

 | 属性名 | 描述 | 格式 |
 | ------ | ------ | ------ |
 | Deep| 是否开启深度刷盘，取值0/1/-1, 0表示不开启，1表示开启，-1表示采用服务器端默认配置。缺省为1。<br/> Deep取值兼容bool类型。 | Deep:1 |
 | Block | 持久化期间是否阻塞所有的变更操作，取值true/false，缺省为false。 | Block:false |
 | CollectionSpace | 指定持久化的集合空间名称，字符串类型。<br/>如果指定该参数，则只会持久化该集合空间，否则会持久化所有的集合空间和日志文件。<br/> |  CollectionSpace:"foo" |
 | Location Elements | 命令位置参数项，详细见 **[命令位置参数](reference/Sequoiadb_command/location.md)** | GroupName:"db1" |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。该操作主要的异常如下：

  *  **SDB_BACKUP_HAS_ALREADY_START**(-67)  
     与离线备份任务冲突，可以关闭'阻塞模式'进行重试。  
  *  **SDB_REBUILD_HAS_ALREADY_START**(-149)  
     与本地重建任务冲突，可以关闭'阻塞模式'进行重试。  
  *  **SDB_DMS_STATE_NOT_COMPATIBLE**(-148)  
     与其它阻塞操作冲突（如其它sync操作），可以关闭'阻塞模式'进行重试。
  *  **SDB_DMS_CS_NOTEXIST**(-34)  
     指定集合空间不存在。

更多错误可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##版本信息##
2.8及以上版本

##示例##

1. 对全系统所有集合空间和日志进行深度持久化

  ```lang-javascript
  > db.sync()
  ```

2. 对指定集合空间"foo"进行深度持久化

  ```lang-javascript
  > db.sync( { CollectionSpace : "foo" } )
  ```

3. 对指定数据组"group1"进行深度加阻塞的持久化

  ```lang-javascript
  > db.sync( { GroupName : "group1", Block : true } )
  ```
