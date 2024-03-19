##名称##

invalidateFsCache - 清除节点的文件系统缓存

##语法##

**db.invalidateFsCache( [options], [expiredTime] )**

##类别##

Sdb

##描述##

该函数用于清除节点的文件系统缓存。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |
| expiredTime | String | 缓存过期时间。过期的缓存将会被清理。格式为整数或整数加时间单位，时间单位可以是小时（h），分钟（m）或秒（s）。如果整数不带单位，则视时间单位为'h'。例如 "72h" 表示 72 个小时。特殊地，"" 表示立即过期。默认为 ""。 | 否 |

> **Note:**
>
> 当不指定 options 时，作用域为所有数据节点。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.4.12 及以上版本

##示例##

* 清除集群所有文件缓存。

    ```lang-javascript
    > db.invalidateFsCache()
    ```
 
* 清除数据组'group1'中超过 72 小时未访问的文件缓存。

    ```lang-javascript
    > db.invalidateFsCache( { GroupName: 'group1' }, "72" )
    ```
 
[^_^]:
     本文使用的所有引用及链接

[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
