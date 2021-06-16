##名称##

getDomain - 获取指定域。

##语法##
**db.getDomain( \<name\> )**

##类别##

Sdb

##描述##

该函数用于获取指定域。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 域名 | 是 |

> **Note:**
>
> 不能获取系统域 SYSDOMAIN。

##返回值##

函数执行成功时，将返回一个 SdbDomain  类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

获取一个之前创建的域。

```lang-javascript
> var domain = db.getDomain( 'mydomain' )
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md