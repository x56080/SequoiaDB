##名称##

limit - 控制查询返回的记录条数

##语法##

**query.limit( \<num\> )**

##类别##

SdbQuery

##描述##

控制查询返回的记录条数。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述                       | 是否必填 |
| ------ | -------- | ------ | -------------------------- | -------- |
| num    | int      | ---    | 自定义返回结果集的记录条数 | 是       |

>**Note:**  

>query.limit() 方法的定义格式包含 num 参数，它是 int 类型。如果不设定 num 的内容，相当于返回所有的结果集记录。如果结果集的记录数小于 num，按实际的记录数返回，如果结果集的记录数大于 num，则只返回前 num 条记录。

##返回值##

返回结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

选择集合 employee 下 age 字段值大于10的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），并只返回前面2条记录。

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).limit( 2 )
{
  "_id": {
    "$oid": "5813035cc842af52b6000009"
  },
  "name": "Tom",
  "age": 11
}
{
  "_id": {
    "$oid": "58130372c842af52b600000a"
  },
  "name": "Jack",
  "age": 12
}
```

