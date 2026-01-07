##名称##

tellPosition - 返回文件游标的偏移量

##语法##

**file.tellPosition()**

##类别##

File

##描述##

返回文件游标的偏移量。

##参数##

无

##返回值##

文件游标的偏移量。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.4.16 和 v5.8.6 及以上版本

##示例##

* 打开一个文件，获取文件游标的偏移量

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.txt" )
    > file.tellPosition()
    0
    ```

* 往文件中写入内容，获取文件游标的偏移量

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.txt" )
    > file.write( "sample" )
    > file.tellPosition()
    6
    ```

* 移动文件游标，获取文件游标的偏移量

    ```lang-javascript
    > file.seek(2)
    > file.tellPosition()
    2
    ```

