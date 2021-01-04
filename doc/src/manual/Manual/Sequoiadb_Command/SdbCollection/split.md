##语法##
***db.collectionspace.collection.split( \<source group\>, \<target group\>, \<percent\> )***  
***db.collectionspace.collection.split( \<source group\>, \<target group\>, \<condition\>, [endcondition] )***

在至少存在两个分区组的环境下，将数据记录按指定的条件切分到不同的分区组中，目标分区组必须属于集合空间所属的域。该操作为同步操作，直到数据切分完成才返回。

##参数描述##
###范围切分###
db.collectionspace.collection.split( \<source group\>, \<target group\>, \<condition\>, [endcondition] )

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| source group | string | 源分区组。 | 是 |
| target group | string | 目标分区组。 | 是 |
| condition | Json 对象 | 范围切分条件。 | 是 |
| endcondition | Json 对象 | 结束范围条件。| 可选 |
> **Note:**
>
>  1. Range 分区使用精确条件，而 Hash 分区使用 Partition（分区数）条件。结束条件不选时默认为切分源当前包含的最大数据范围。  
>  2. 如果指定分区键字段为降序时，如：{groupingKey:{<字段1>:&lt;-1&gt;}，condition（或 Partition）中的起始条件中的范围应该大于终止条件中的范围。Hash 分区使用的 Partition（分区数）必须为整型，不能为其他的类型。

###百分比切分###
db.collectionspace.collection.split( \<source group\>, \<target group\>, \<percent\> )

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| source group | string | 源分区组。 | 是 |
| target group | string | 目标分区组。 | 是 |
| percent | double | 百分比切分条件，取值：(0, 100] | 是 |

> **Note:**
>
>  1. Range分区需要保证源分区组中含有数据，即集合不为空；  
>  2. 百分比不能为0.

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误信息码。

##错误##

[错误码][error_code]

##示例##

* Hash 分区范围切分

 ```lang-javascript
 > db.foo.bar.split( "group1",  "group2", { Partition: 10 }, { Partition: 20 } )
 ```

* Range 分区范围切分

 ```lang-javascript
 > db.foo.bar.split( "group1",  "group2", { a: 10 },  { a: 10000 } )
 ```

* 百分比切分

 ```lang-javascript
 > db.foo.bar.split( "group1",  "group2",  50 ) 
 ```


[^_^]:
    本文使用的所有引用及链接
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[error_code]:manual/Manual/Sequoiadb_error_code.md