##语法##
***db.collectionspace.collection.remove\(\[cond\],\[hint\]\)***

删除集合中的记录。

##参数描述##
| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| cond   | Json 对象| 选择条件。为空时，删除所有记录，不为空时，删除符合条件的记录。 | 否 |
| hint   | Json 对象| 指定访问计划。 | 否 |

> **Note:**
>
> * 参数`cond`和`hint`的用法与[find()](reference/Sequoiadb_command/SdbCollection/find.md)的相同。

##返回值##
*  成功返回详细结果信息（BSONObj 对象），结构如下：

 ```lang-json
 {
		DeletedNum : <INT64>  成功删除的记录数
 }
 ```

* 出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。错误信息对象包括详细结果信息。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 删除集合所有记录

 ```lang-javascript
 > db.foo.bar.remove()
 ```

* 按访问计划删除匹配 cond 条件的记录，如下操作按照索引名为“myIndex”的索引遍历集合中的记录，在遍历得到的记录中删除符合条件 age 字段值大于等于20的记录。

 ```lang-javascript
 > db.foo.bar.remove( { age: { $gte: 20 } }, { "": "myIndex" } )
 ```

