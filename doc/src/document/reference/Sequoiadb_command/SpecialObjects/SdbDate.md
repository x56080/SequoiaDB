
YYYY-MM-DD形式的日期。范围：0000-01-01至9999-12-31。

数据类型的介绍可参考 [数据类型](data_model/datatype/datatype.md)。

##Json格式##

* 语法

  *{ "$date": \<date\> }*

* 参数描述

  | 参数名 | 参数类型 | 描述                     | 是否必填 |
  | ------ | -------- | ------------------------ | -------- |
  | date   | 字符串   | YYYY-MM-DD格式的字符串。 | 是       |

>  **Note:**
>
>  SdbDate 的范围将在 SequoiaDB v2.8.2 版本改为 0000-01-01 至 9999-12-31。之前的 SequoiaDB v2.8 版本为 1900-01-01 至 9999-12-31。

##函数格式##

* 语法： 
 
  *SdbDate( [data] )*

* 参数描述

  | 参数名 | 参数类型 | 描述                                                 | 是否必填 |
  | ------ | -------- | ---------------------------------------------------- | -------- |
  | date   | 字符串   | YYYY-MM-DD格式的字符串。<br> 为空时，生成当天的日期。| 否       |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](reference/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ | 
| -6     | 参数错误    | 请参考示例。 | 

##示例##

插入日期类型的记录

```lang-javascript
> db.foo.bar.insert( { date: { $date: "2015-03-13" } } )
> db.foo.bar.insert( { date: SdbDate( "2015-03-13" ) } )
> db.foo.bar.insert( { date: SdbDate() } )
```
