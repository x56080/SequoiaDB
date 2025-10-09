##名称##

before - 设置 search() 结果上下文的上文条数

##语法##

**diaglog.search().before(\<num\>)**

##类别##

DiagLog

##描述##

设置 search() 结果上下文的上文条数。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述            | 是否必填 |
| -------- | -------- | ------ | --------------- | -------- |
| num      | int      | ---    | 返回前面 num 条日志  | 是       |


##返回值##

DiagLog 对象。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v5.8 及以上版本

##示例##

* 新建一个 DiagLog 对象

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* 搜索最近报错 -79 错误的日志，并包含错误日志的前一条日志，限制返回 10 条结果。

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).before( 1 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
