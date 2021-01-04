
##语法##
***db.createRG( \<name\> )***

新建一个复制组。创建后系统自动为复制组分配一个GroupId。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 分区组名，同一个数据库对象中，分区组名唯一。 | 是 |


> **Note:**
>
> * 复制组名不能是空串，不能含点（.）或者美元符号（$），并且长度不能超过127B。


##返回值##

返回新建复制组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 新建名为 “group1” 的复制组

 ```lang-javascript
 > db.createRG( "group1" )
 ```
