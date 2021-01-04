
##语法##
***db.getDomain( \<name\> )***

获取指定域。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 域名 | 是 |

> **Note:**
>
> 不能获取系统域 SYSDOMAIN。

##返回值##

返回指定域的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 获取一个之前创建的域。

 ```lang-javascript
 > var domain = db.getDomain( 'mydomain' )
 ```
