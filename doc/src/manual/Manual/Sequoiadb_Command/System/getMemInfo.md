##名称##

getMemInfo - 获取内存信息

##语法##

**System.getMemInfo()**

##类别##

System

##描述##

获取内存信息

##参数##

无

##返回值##

返回内存信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取内存信息

```lang-javascript
> System.getMemInfo()
{
<<<<<<< HEAD
    "Size": 5967,
    "Used": 2919,
    "Free": 384,
=======
    "Size": 7982,
    "Used": 4945,
    "Free": 192,
    "Available": 2713,
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    "Unit": "M"
}
```