##语法##
***db.forceSession( \<sessionID\>, [options] )***

终止指定会话的当前操作。当不指定节点信息时，默认为对自身的会话进行操作。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| sessionID | int | 会话编号 | 是 |
| options | Json对象 | **[命令位置参数](reference/Sequoiadb_command/location.md)** | 否 |

> **Note:**
>
> * 只有用户会话可以被终止。
> * 会话编号可以通过[list()](reference/Sequoiadb_command/Sdb/list.md)或[snapshot()](reference/Sequoiadb_command/Sdb/snapshot.md)获取。
> * options参数只在协调节点生效。
> * 如果终止的会话是当前会话，连接会被断开，不能再执行操作。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。该操作主要的异常如下：

  *  **SDB_PMD_SESSION_NOT_EXIST**(-62)  
     指定会话不存在：  
     1）需要确认指定会话对应的节点是否正确；  
     2）该节点上是否存在指定会话。
  *  **SDB_PMD_FORCE_SYSTEM_EDU**(-63)  
     系统会话不能被终止。

更多错误可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 终止当前节点编号为30的会话。

 ```lang-javascript
 // 连接节点
 > db = new Sdb( "localhost", 11810 )
 // 终止指定会话
 > db.forceSession( 30 )
 ```

* 终止节点ID为1000上的编号为30的会话。

 ```lang-javascript
 // 连接节点
 > db = new Sdb( "localhost", 11810 )
 // 终止编号为30的会话
 > db.forceSession( 30, {NodeID:1000} )
 ```
