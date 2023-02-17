##名称##

read - 读取文本文件

##语法##

**file.read(\[size\])**

##类别##

File

##描述##

该函数用于读取指定的文本文件。

##参数##

size（ *number，选填*）

从当前的文件游标位置开始，需要读取的字节数，默认值为 1024

##返回值##

函数执行成功时，将返回读取的文件内容。
     
函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

- 从文件 `file` 的游标位置开始，读取 1024 字节的内容

    ```lang-javascript
    > var file = new File("/opt/sequoiadb/file")
    > file.read()
    ```

- 与 [getSize][getSize] 搭配使用，可读取文件 `file` 游标位置之后的全部内容

    ```lang-javascript
    > var file = new File("/opt/sequoiadb/file")
    > file.read(file.getSize("/opt/sequoiadb/file"))
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getSize]:manual/Manual/Sequoiadb_Command/File/getSize.md