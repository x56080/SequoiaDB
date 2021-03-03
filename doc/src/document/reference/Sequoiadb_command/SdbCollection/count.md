##语法##
***db.collectionspace.collection.count\(\[cond\]\)***

统计指定集合空间下指定集合的记录总数。

##参数描述##

|参数名 |  参数类型   | 描述   |是否必填 |
| ------| ----------- | ------ | --------|
| cond  | Json 对象   | 选择条件。为空时，统计集合下所有的记录总数；不为空时，统计符合条的记录总数。  | 否|

> **Note:**
>
> * 指定cond字段时可使用对应[匹配符](reference/operator/match_operator/overview.md)查询。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 统计集合 bar 所有的记录数，即不指定参数 cond

 ```lang-javascript
 > db.foo.bar.count()
 ```

* 统计符合条件 name 字段的值为“Tom”且 age 字段的值大于25的记录数

 ```lang-javascript
 > db.foo.bar.count( { name: "Tom", age: { $gt: 25 } } )
 ```
