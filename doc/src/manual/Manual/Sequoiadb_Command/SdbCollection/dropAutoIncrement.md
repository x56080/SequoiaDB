
##语法##
***db.collectionspace.collection.dropAutoIncrement\(\<name|names\>\)***

在指定集合中删除一个或多个自增字段。

## 参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| name&#124;names | String | 自增字段名。name 是一个字段的名称，names 是多个字段的名称。 | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 删除一个自增字段

 ```lang-javascript
 > db.sample.employee.dropAutoIncrement( "studentID" )
 ```

* 删除多个自增字段

 ```lang-javascript
 > db.sample.employee.dropAutoIncrement( [ "comID", "innerID" ] )
 ```

