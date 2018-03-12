枚举数据库中所有的集合。

##语法##
***list collections***

##参数##
无

##返回值##
数据库中所有的集合。

##示例##

   * 返回数据库所有的集合。

   ```lang-javascript
   > db.exec("list collections") 
   { "Name": "foo.bar" }
   { "Name": "foo.bar2" }
   Return 2 row(s).
   Takes 0.4929s.
   ```