##语法##
***db.collectionspace.getCL( \<name\> )***

获取指定集合对象的引用。

## 参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ |------ |
| name | string | 集合名，在同一个集合空间中，集合名必须唯一。 | 是 |

##返回值##

返回集合对象的引用，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -23			| 集合不存在    | 使用列表查看集合是否存在	|

[错误码](reference/Sequoiadb_error_code.md)

##格式##

getCL() 方法的定义格式必须指定 name 参数，并且 name 的值在集合空间中存在，否则操作异常。

 ```
{ "name": "<集合名>" }
 ```

> **Note:**

> * name 的值不能是空串、含点（.）或者美元符号（$），并且长度不能超过127B，否则操作失败。
> * 集合名必须在集合空间中存在，否则操作异常。

##示例##

* 返回集合空间 foo 下集合 bar 的引用，假定集合存在

 ```lang-javascript
> db.foo.getCL( "bar" )
 ```
