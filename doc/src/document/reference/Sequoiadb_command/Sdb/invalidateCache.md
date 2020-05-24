##语法##
***db.invalidateCache( [options] )***

清除节点的缓存信息。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | Json对象 | **[命令位置参数](reference/Sequoiadb_command/location.md)** | 否 |

> **Note:**
>
> 当不指定 options 时，作用域为当前协调节点、所有数据节点、所有编目节点。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 清除当前协调节点和数据组‘group1’的缓存信息。

 ```lang-javascript
 > db.invalidateCache( { GroupName: 'group1' } )
 ```
 
* 清除当前协调节点的缓存信息。

 ```lang-javascript
 > db.invalidateCache( { Global: false } )
 ```

* 清除所有协调节点的缓存信息。

 ```lang-javascript
 > db.invalidateCache( { GroupName: 'SYSCoord' } )
 ```