选择字段 \<field_name\> 的值存在且不为 null 的记录。

##语法##
***\<field_name\> is not null***

##参数##
| 参数名     | 参数类型 | 描述     | 是否必填 |
|------------|----------|----------|----------|
| field_name | string   | 字段名。 | 是       |

##返回值##
无。

## 示例##

   * 集合 foo.bar 中原始记录。

   ```
   { a: 1 }
   { a: null }
   { b: 1 }
   ```

   * 查询 a 字段值存在且不为 null 的记录。

   ```lang-javascript
   > db.exec('select * from foo.bar where a is not null')
   {
     "_id": {
       "$oid": "598d0b57a6e2e2fd65000000"
     },
     "a": 1
   }
   Return 1 row(s).
   Takes 0.005813s.
   ```
