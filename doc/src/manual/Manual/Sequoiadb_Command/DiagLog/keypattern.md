##名称##

keypattern - 设置 search() 中搜索的关键字

##语法##

**diaglog.search().keypattern(\<keyword\>)**

##类别##

DiagLog

##描述##

设置 search() 中搜索的关键字。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述            | 是否必填 |
| -------- | -------- | ------ | --------------- | -------- |
| keyword  | string   | ---    | 在日志中搜索的关键字 keyword | 是       |


##返回值##

DiagLog 对象。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v5.8 及以上版本

##示例##

* 新建一个 Sdb 对象

    ```lang-javascript
    > var db = new Sdb()
    ```

* 新建一个 DiagLog 对象

    ```lang-javascript
    > var diaglog = new DiagLog()
    ```

* 搜索日志内容中包含 'failed' 的日志，限制返回 10 条结果。

    ```lang-javascript
    > diaglog.search().keypattern( 'failed' ).limit( 10 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md