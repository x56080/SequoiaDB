##语法##
***db.collectionspace.collection.createAutoIncrement\(\<option|options\>\)***

在指定集合中创建一个或多个自增字段。

## 参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| option&#124;options | Json 对象 | 自增字段参数。option 是一个字段的参数，options 是多个字段的参数。 | 是 |

option中的具体参数请参见[自增字段属性](manual/Distributed_Engine/Architecture/Data_Model/sequence.md)

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

## 示例##

* 创建一个自增字段，其值总是由服务端生成，忽略客户端设置。

 ```lang-javascript
 > db.foo.bar.createAutoIncrement( { Field: "studentID", Generated: "always" } )
 ```

* 创建两个自增字段

 ```lang-javascript
 > db.foo.bar.createAutoIncrement( [ { Field: "comID" }, { Field: "innerID" } ] )
 ```

