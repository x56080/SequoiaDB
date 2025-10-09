##名称##

core - 设置 collect() 收集 core 文件

##语法##

**diaglog.collect().core()**

##类别##

DiagLog

##描述##

设置 collect() 收集 core 文件。

##参数##

无

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

* 收集 core 文件。

    ```lang-javascript
    > diaglog.collect().core()
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```
