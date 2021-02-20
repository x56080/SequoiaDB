##语法##
***db.collectionspace.collection.remove\(\[cond\], \[hint\], \[options\]\)***

删除集合中的记录。

##参数描述##
| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| cond   | Json 对象| 选择条件。为空时，删除所有记录，不为空时，删除符合条件的记录。 | 否 |
| hint   | Json 对象| 指定访问计划。 | 否 |
| options| Json 对象| 可选项，详见options选项说明。| 否 |

##options选项##

| 参数名          | 参数类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| JustOne         | bool     | 为 true 时，将只更新一条符合条件的记录。<br>为 false 时，将会更新所有符合条件的记录。| false  |

> **Note:**
>
> * 参数`cond`和`hint`的用法与 [find()][find] 的相同。
>
> * `JustOne`为 true 时，只能在单个分区、单个子表上执行。

##返回值##
*  成功返回详细结果信息（BSONObj 对象），结构如下：

 ```lang-json
 {
		DeletedNum : <INT64>  成功删除的记录数
 }
 ```

* 出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误信息码。错误信息对象包括详细结果信息。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码][error_code]。
  
| 错误码   | 可能的原因               | 解决方法                                     |
| -------- | ------------------------ | -------------------------------------------- |
| -346     | JustOne 跨多个分区或者多个子表 | 修改匹配条件，或者不使用 JustOne |

##示例##

* 删除集合所有记录

 ```lang-javascript
 > db.sample.employee.remove()
 ```

* 按访问计划删除匹配 cond 条件的记录，如下操作按照索引名为“myIndex”的索引遍历集合中的记录，在遍历得到的记录中删除符合条件 age 字段值大于等于20的记录。

 ```lang-javascript
 > db.sample.employee.remove( { age: { $gte: 20 } }, { "": "myIndex" } )
 ```


[^_^]:
    本文使用的所有引用及链接
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
