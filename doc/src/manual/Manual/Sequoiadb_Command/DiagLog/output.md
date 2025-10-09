##名称##

output - 设置 search() 和 analyze() 的输出路径

##语法##

**diaglog.search().output(\<path\>)**

**diaglog.analyze().output(\<path\>)**

##类别##

DiagLog

##描述##

设置 search() 和 analyze() 的输出路径。

##参数##

| 参数名     | 参数类型 | 默认值             | 描述            | 是否必填 |
| ---------- | -------- | ------------------ | --------------- | -------- |
| path   | string   | ---          | 设置 search() 和 analyze() 的输出路径，需为绝对路径          | 是       |

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

* 把 collect() 的结果文件放到指定目录。

    ```lang-javascript
    > diaglog.collect().all().path( '/home/sdbadmin/collect' )
    /home/sdbadmin/collect/diaglog_20250101_120101
    ```

* 指定 search() 搜索的目录，结果输出到指定目录。

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).path( '/home/sdbadmin/collect/diaglog_20250101_120101' ).output( '/home/sdbadmin/search' )
    /home/sdbadmin/search/result
    ```

* 指定 analyze() 分析的目录，结果输出到指定目录。

    ```lang-javascript
    > diaglog.analyze().path( '/home/sdbadmin/collect/diaglog_20250101_120101' ).output( '/home/sdbadmin/search' )
    /home/sdbadmin/analyze/result
    ```

