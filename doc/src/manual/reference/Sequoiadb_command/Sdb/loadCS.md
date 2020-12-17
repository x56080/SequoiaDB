##语法##

***db.loadCS( \<csName\>, [options] )***

##类别##

Sdb

##描述##

加载集合空间到内存。

##参数##

| 参数名  | 参数类型 | 默认值  | 描述               | 是否必填 |
| ------- | -------- | ------- | ------------------ | -------- |
| csName  | string   | ---     | 集合空间名         | 是       |
| options | JSON     | 空      | [命令位置参数](reference/Sequoiadb_command/location.md) | 否       |

>**Note:**

>只有在连接协调节点时，options 参数才会生效

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 查询数据。（假定存在集合空间 “foo”，而且当前 SequoiaDB 是独立模式启动的）

   ```lang-javascript
   > db.foo.bar.find()
   {
      "_id": {
        "$oid": "5d36c9d5c6b1cee56abefc7e"
      },
      "name": "fang",
      "age": 18
   }
   ```  

* 卸载内存中的集合空间 “foo”。

   ```lang-javascript
   > db.unloadCS( "foo" )
   ```

* 查询数据。

   ```lang-javascript
   > db.foo.bar.find()
   uncaught exception: -34
   Collection space does not exist
   ``` 

* 加载集合空间 “foo” 到内存中。

   ```lang-javascript
   > db.loadCS( "foo" )
   ```

* 再次查询数据。

   ```lang-javascript
   > db.foo.bar.find()
   {
      "_id": {
        "$oid": "5d36c9d5c6b1cee56abefc7e"
      },
      "name": "fang",
      "age": 18
   }
   ```  