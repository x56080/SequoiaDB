##语法##
***query.skip( [num] )***

指定结果集从哪条记录开始返回。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| num | int | 自定义结果集从哪条记录返回。 | 否 |

> **Note：**  
> query.skip() 方法的定义格式包含 num 参数，它是 int 类型。如果不设定 num 的内容或者设定 num 的值为0，相当于返回所有的结果集；如果想从结果集的第3条记录开始返回，可是设置 num 的值等于2。

##返回值##

返回结果集的游标，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

选择集合 bar 下 age 字段值大于10的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），从第5条记录开始返回，即跳过前面的四条记录

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).skip(4)
```

> **Note：**  
> 如果结果集的记录数小于5，那么无记录返回；如果结果集的记录数大于5，则从第5条开始返回。

