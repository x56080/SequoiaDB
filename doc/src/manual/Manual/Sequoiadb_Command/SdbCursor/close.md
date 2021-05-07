##名称##

close - 关闭当前游标

##语法##

**cursor.close()**

##类别##

SdbCursor

##描述##

该函数用于关闭当前游标，关闭后游标将无法获取记录。

##参数##

无

##返回值##

关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。
函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.0 及以上版本

##示例##

* 插入 10 条记录

    ```lang-javascript
    > for(i = 0; i < 10; i++) { db.sample.employee.insert( {a: i} ) }
    ```

* 查询集合 sample.employee 的所有记录

    ```lang-javascript
    > var cur = db.sample.employee.find()
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

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
