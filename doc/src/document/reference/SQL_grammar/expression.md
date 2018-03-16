内置 SQL 支持算术表达式、比较表达式。

## 算术表达式 ##

### 格式 ###

***\<field1_name\> \<[mathematical_operator](reference/SQL_grammar/operator.md#算术运算符)\> \<value\>***

>**Note:** 

> 支持复合运算，如 select \( a + 1 \) * \( a - 1 \), b + 1 from foo.bar

### 示例 ###

age字段值加一

```
age + 1
```

取集合中age字段，age值加1

```lang-javascript
> db.exec( "select age + 1 from foo.bar" )
{
  "age": 12
}
```

## 比较表达式 ##

### 格式 ###

***\<field1_name\> \<[comparison_operator](reference/SQL_grammar/operator.md#比较运算符)\> \<value | field2_name\>***

***\<field1_name\> \<[comparison_operator](reference/SQL_grammar/operator.md#比较运算符)\> \<value\> \<[logical_operator](reference/SQL_grammar/operator.md#逻辑运算符)\> \<field1_name\> \<comparison_operator\> \<value\>***

### 示例 ###

*  age字段大于11

   ```
   age > 11
   ```

   查询匹配age大于11的记录。

   ```lang-javascript
    > db.exec( "select * from foo.bar where age > 11" )
    {
      "_id": {
      "$oid": "5aa3330fdc5673331f000000"
      },
      "name": "Lucy",
      "age": 12
    }
   ```
*  age字段大于等于11，且name为Harry

   ```
   age >= 11 AND name = 'Harry'
   ```
  
   查询匹配age大于等于11，且name为Harry的记录。
  
   ```lang-javascript
    > db.exec( "select * from foo.bar where age >= 11 AND name = 'Harry'" )
    {
      "_id": {
      "$oid": "5aa35dbedc5673331f000001"
      },
      "name": "Harry",
      "age": 11
    }
   ```
