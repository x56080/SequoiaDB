##语法##
***db.collectionspace.collection.listLobs\(\)***

枚举集合中的大对象。

> **Note:**
>
> 此方法暂不支持排序等查询操作。

##参数##

无

##返回值##

返回游标。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

## 示例##

* 枚举 foo.bar 中的所有大对象

 ```lang-javascript
 > db.foo.bar.listLobs()
 ```
