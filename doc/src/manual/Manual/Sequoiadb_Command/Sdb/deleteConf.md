##名称##

deleteConf - 删除配置

##语法##

**db.deleteConf( \<config\>, [options] )**

##类别##

Sdb

##描述##

该函数用于恢复配置默认值，进行配置动态生效，并将配置从配置文件中删除。重启生效的配置需重启后生效，禁止修改的配置则不允许修改。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| config | Json对象 |配置参数，包含配置名和占位符，例如：{ preferredinstance:1, diaglevel:1 }，其中 1 没有特殊含义，仅作为占位符出现。| 是 |
| options| Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

> **Note:**
>
> * 参照[数据库配置](manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md)页面获取配置的动态生效、重启生效和禁止修改信息。
> * 重启生效和禁止修改配置的详细信息会通过错误信息返回值通知。
> * 若配置的默认值和数据库当前值相同，则重启生效和禁止修改配置不会报错。
> * 无命令位置参数时，缺省对所有节点生效，即使用 {Global:true} 的命令位置参数。
> * 可以通过 [snapshot](manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md) 获取指定节点的当前配置。


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

2.9及以上版本

##示例##

* 删除数据节点 20000 上的 diaglevel 参数，恢复其默认值。

    ```lang-javascript
    // 连接协调节点
    > db = new Sdb( "localhost", 11810 )
    > db.deleteConf( { diaglevel:1 }, { GroupName:"db1", ServiceName:"20000" } )
    ```

* 删除数据组 db2 上所有数据节点的 preferredinstance 和 diaglevel 参数，恢复默认值。

    ```lang-javascript
    // 连接协调节点
    > db = new Sdb( "localhost", 11810 )
    > db.deleteConf( { preferredinstance:1, diaglevel:1 }, { GroupName:"db2" } )
    ```

* 报错时获取详细错误信息。


    ```lang-javascript
    // 连接协调节点
    > db = new Sdb( "localhost", 11810 )
    // 进行参数配置，报错
    > db.deleteConf( { transactionon:1 }, { ServiceName:"20000" } )
      (nofile):0 uncaught exception: -264
      One or more nodes did not complete successfully
      Takes 0.009322s.
    // 获取详细报错信息，了解到 transactionon 参数需要重启生效
    > getLastErrObj()
       {
           "errno": -264,
           "description": "One or more nodes did not complete successfully",
           "detail": "",
           "ErrNodes": [
           {
               "NodeName": "ubuntu-zwb:20000",
               "GroupName": "db1",
               "Flag": -322,
               "ErrInfo": {
               "errno": -322,
               "description": "Some configuration changes didn't take effect",
               "detail": "Config 'transactionon' require(s) restart to take effect."
               }
           }
           ]
       }
    ```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
