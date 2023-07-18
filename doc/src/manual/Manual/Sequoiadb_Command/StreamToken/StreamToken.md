##名称##

StreamToken - 生成数据流的位置信息

##语法##

**StreamToken()**

**StreamToken(\<token\>)**

##类别##

StreamToken

##描述##

该函数用于生成数据流的位置信息。可以从最新的位置开始读取数据流，也可以指定的位置开始读取数据流。

##参数##

token（ *string，可选* ）

表示指定位置开始读取数据流的位置信息字符串

##返回值##

函数执行成功时，返回生成的 StreamToken 对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v7.2.2 及以上版本

##示例##

生成最新的位置开始读取数据流位置信息

```lang-javascript
> var token = new StreamToken()
```

生成指定位置开始读取数据流位置信息

```lang-javascript
> var token = SdbStreamToken( “00010000000003e800000000000000000000000000000ac80000000100000000” )
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md