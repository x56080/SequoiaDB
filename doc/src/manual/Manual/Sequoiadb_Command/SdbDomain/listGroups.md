
##语法##
***domain.listGroups()***

获取指定域中所有的复制组。

##参数描述##

无

##返回值##

返回指定域下的复制组信息，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。


##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 获取指定域下的复制组

```lang-javascript
  > domain.listGroups()
 {
   "_id": {
     "$oid": "5b92291ec5e807b5e32582cc"
   },
   "Name": "mydomain",
   "Groups": [
     {
       "GroupName": "db1",
       "GroupID": 1000
     },
     {
       "GroupName": "db2",
       "GroupID": 1001
     }
   ],
   "AutoSplit": true
 }
```