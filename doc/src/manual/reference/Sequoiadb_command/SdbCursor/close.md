##语法##
***cursor.close()***

关闭当前游标，当前游标不再可用。

##参数描述##
无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 插入10条记录

 ```lang-javascript
 > for(i = 0; i < 10; i++) { db.foo.bar.insert( {a: i} ) }
 ```

* 查询集合 foo.bar 的所有记录

 ```lang-javascript
 > var cur = db.foo.bar.find()
 ```

* 使用游标取出一条记录

 ```lang-javascript
> cur.next()
 {
      "_id": {
      "$oid": "53b3c2d7bb65d2f74c000000"
      },
      "a": 0
 }
 ```

* 关闭游标

 ```lang-javascript
> cur.close()
 ```

* 再次获取下一条记录，无结果返回

 ```lang-javascript
> cur.next()
 ```


