##名称##

getCataRG - 获取编目分区组的引用。 

##语法##

***db.getCataRG()***

##类别##

Sdb

##描述##

获取编目分区组的引用。

##参数##

无

##返回值##

返回编目分区组的引用。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。
关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][Sequoiadb_error_code]。

##示例##

- 获取编目分区组引用。

```lang-javascript
> var rg = db.getCataRG()
```


[^_^]:
     本文使用的所有引用和链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md