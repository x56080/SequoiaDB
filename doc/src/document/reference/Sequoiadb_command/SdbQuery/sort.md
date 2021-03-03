##语法##
***query.sort( \<sort\> )***

对结果集按指定字段排序。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
|sort |	Json 对象 | 对结果集按指定字段排序。字段名的值为1或者-1，1代表升序；-1代表降序。 如果不设定 sort 则表示不对结果集做排序。 | 否 |

> **Note：**  
> 1. 当 find() 方法使用 sel 选项，若该选项没有包含 sort() 指定的排序字段，此时 sort() 方法设置的排序无意义，从而被自动忽略。  
> 2. 如果不设定 sort 的内容，则对返回的结果集不排序。  
> 格式：```{ <字段名1>: <-1|1>, <字段名2>: <-1|1>, ... } ```  

##返回值##

返回结果集的游标，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

- 返回集合 bar 中 age 字段值大于20的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序。

```lang-javascript
  > db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 } )
```

> **Note:**  
> 通过 [find()](reference/Sequoiadb_command/SdbCollection/find.md) 方法，我们能任意选择我们想要返回的字段名，在上例中我们选择了返回记录的 age 和 name 字段，此时用 sort() 方法时，只能对记录的 age 或 name 字段排序。而如果我们选择返回记录的所有字段，即不设置 find 方法的 sel 参数内容时，那么 sort() 能对任意字段排序。

- 指定一个无效的排序字段。

```lang-javascript
  > db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
```

> **Note:**  
> 因为“sex”字段并不存在 find() 方法的 sel 选项 {age:"",name:""} 中，所以 sort() 指定的排序字段 {"sex":1} 将被忽略。
