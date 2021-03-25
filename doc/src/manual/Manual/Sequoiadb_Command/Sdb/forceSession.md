##名称##

forceSession - 终止指定会话的当前操作

##语法##
**db.forceSession( \<sessionID\>, [options] )**

##类别##

Sdb

##描述##

该函数用于终止指定会话的当前操作。当不指定节点信息时，默认为对自身的会话进行操作。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| sessionID | int | 会话编号 | 是 |
| options | Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

> **Note:**
>
> * 只有用户会话可以被终止。
> * 会话编号可以通过[list()](manual/Manual/Sequoiadb_Command/Sdb/list.md)或[snapshot()](manual/Manual/Sequoiadb_Command/Sdb/snapshot.md)获取。
> * options参数只在协调节点生效。
> * 如果终止的会话是当前会话，连接会被断开，不能再执行操作。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`forceSession()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -62 | SDB_PMD_SESSION_NOT_EXIST | 指定会话不存在。| 检查指定会话是否存在。|
| -63 | SDB_PMD_FORCE_SYSTEM_EDU | 系统会话不能被终止|检查磁盘是否损坏或重新执行备份并删除该备份文件|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本


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

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md