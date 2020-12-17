##语法##

***query.arrayAccess( \<index\> )***

##类别##

SdbQuery

##描述##

先将结果集保存到数组中，然后获取指定下标的记录，下标从 0 开始。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述               | 是否必填 |
| ------ | -------- | ------ | ------------------ | -------- |
| index  | int      | ---    | 要访问的记录的下标 | 是       |

##返回值##

返回指定下标的记录。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 返回数组中下标为 0 的记录

  ```lang-javascript
  > db.foo.bar.find().arrayAccess(0)
  {
      "_id": {
        "$oid": "5cf8aef75e72aea111e82b38"
      },
      "name": "tom",
      "age": 20
  }
  ```
