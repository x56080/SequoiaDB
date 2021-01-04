
##语法##
***db.getRG( \<name\> | \<id\> )***

获取指定复制组。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 复制组名，同一个数据库对象中，复制组名唯一。| name 和 id 任选一个 |
| id | int | 复制组 id，创建复制组时系统自动分配。 | id 和 name 任选一个 |

> **Note:**
>
> * name 字段的值不能使空串，不能含点（.）或者美元符号（$），且长度不超过127B。

##返回值##

返回指定复制组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息 或 通过 [getLastError()][getLastError] 获取错误码。关于错误处理可以参考[常见错误处理指南][faq]。

##示例##

* 指定 name 值，返回复制组 group1 的引用

 ```lang-javascript
 > db.getRG( "group1" )
 ```

* 指定 id 值，返回复制组 group1 的引用（假定 group1 的复制组 id 为1000）

 ```lang-javascript
 > db.getRG( 1000 )
 ```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md