##名称##

diaglevel - 设置 search() 中过滤的日志级别

##语法##

**diaglog.search().diaglevel(\<level\>)**

##类别##

DiagLog

##描述##

设置 search() 中过滤的日志级别。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述            | 是否必填 |
| -------- | -------- | ------ | --------------- | -------- |
| level    | set      | ---    | 可选值 [0-4], 包含更低级别 <br>0: SEVERE<br>1: ERROR<br>2: EVENT<br>3: WARNING<br>4: INFO  | 是       |


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

* 搜索最近报错 -79 错误的日志，限制返回 10 条结果，设置日志级别为 1，结果仅包含 SEVERE 和 ERROR 级别的日志。

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).diaglevel( 1 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
