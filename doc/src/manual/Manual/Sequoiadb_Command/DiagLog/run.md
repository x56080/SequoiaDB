##名称##

run - 以当前设置的参数运行

##语法##

**diaglog.run()**

##类别##

DiagLog

##描述##

以当前设置的参数运行。

##参数##

无

##返回值##

执行的结果。

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

* 设置参数后用 run 运行。

    ```lang-javascript
    > var filename = diaglog.search().error( -79 ).limit( 10 ).run();
    ```
