##语法##

***cursor.arrayAccess( \<index\> )***

***cursor[ \<index\> ]***

先将结果集保存到数组中，然后获取指定下标的记录，下标从 0 开始。

##参数描述##

*   `index` ( *Number*，*必填* )

    要访问的记录的下标。

##返回值##

返回指定下标的记录，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##示例##

* 返回数组中下标为 0 的记录

 ```lang-javascript
> db.foo.bar.find().arrayAccess(0)
{
      "_id": {
      "$oid": "581192bd6db4da2a23000009"
      },
      "a": 9
}
 ```
