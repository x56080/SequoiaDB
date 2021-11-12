##名称##

listCollectionSpaces - 列举域包含的集合空间

##语法##

**domain.listCollectionSpaces()**

##类别##

SdbDomain

##描述##

该函数用于列举指定域包含的集合空间。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取域包含的集合空间信息。

函数执行失败时，将抛异常并输出错误信息。


##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

列举指定域包含的集合空间信息

```lang-javascript
> var domain = db.getDomain('mydomain')
> domain.listCollectionSpaces()
{
  "Name": "sample1"
}
{
  "Name": "sample2"
}
```