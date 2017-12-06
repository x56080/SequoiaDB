选择字段 \<field_name\> 的值为 null，或者不存在的记录。

##语法##
***\<field_name\> is null***

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

   * 查询 a 字段值为 null，或者不存在的记录。

   ```lang-javascript
   > db.exec('select * from foo.bar where a is null')
   {
   "_id": {
      "$oid": "599547f22d8380a914000000"
   },
   "b": 1
   }
   {
   "_id": {
      "$oid": "599548262d8380a914000002"
   },
   "a": null
   }
   Return 2 row(s).
   Takes 0.008678s.
   ```
