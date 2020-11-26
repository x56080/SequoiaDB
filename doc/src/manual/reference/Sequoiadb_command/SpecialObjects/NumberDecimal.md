高精度数，可以保证精度不丢失。

数据类型的介绍可参考 [数据类型](manual/infrastructure/Data_Model/data_type.md)。

##Json格式##

* **格式1**

* 语法

  *{ "$decimal": \<data\> }*

  参数描述

  | 参数名    | 参数类型 | 描述                 | 是否必填 |
  | --------- | -------- | -------------------- | -------- |
  | data      | 字符串   | 存储的数据内容。     | 是       |

* **格式2**

* 语法

  *{ "$decimal": \<data\>, "$precision": [\<precision\>, \<scale\>] }*

  参数描述

  | 参数名    | 参数类型 | 描述                 | 是否必填 |
  | --------- | -------- | -------------------- | -------- |
  | data      | 字符串   | 存储的数据内容。     | 是       |
  | precision | 整数     | 总精度。             | 是       |
  | scale     | 整数     | 小数位精度。         | 是       |
  
##函数格式##

* 语法

  *NumberDecimal( \<data\> [, [ \<precision\>, \<scale\> ] ] )*

  参数描述

  | 参数名    | 参数类型 | 描述             | 是否必填 |
  | --------- | -------- | ---------------- | -------- |
  | data      | 字符串   | 存储的数据内容。 | 是       |
  | precision | 整数     | 总精度。         | 否       |
  | scale     | 整数     | 小数位精度。     | 否       |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](reference/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ | 
| -6     | 参数错误    | 请参考示例。 | 

##示例##

插入高精度类型的记录

```lang-javascript
> db.foo.bar.insert( { a: { $decimal: "100.01" } } )
> db.foo.bar.insert( { a: { $decimal: "100.01", $precision: [ 10, 2 ] } } )
> db.foo.bar.insert( { a: NumberDecimal( "100.01", [ 10, 2 ] ) } )
```