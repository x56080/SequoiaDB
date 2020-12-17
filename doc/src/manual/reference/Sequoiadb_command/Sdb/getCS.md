##语法##
***db.getCS( \<name\> )***

获取指定集合空间

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 集合空间名，同一个数据库对象中集合空间名唯一。 | 是 |

> **Note:**
>
> * name 字段的值不能使空串，不能含点（.）或者美元符号（$），且长度不超过127B。
> * 集合空间在数据库对象中存在。

##返回值##

返回集合空间的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 返回集合空间 foo 的引用，假定 foo 已存在。

 ```lang-javascript
 > db.getCS( "foo" )
 ```
