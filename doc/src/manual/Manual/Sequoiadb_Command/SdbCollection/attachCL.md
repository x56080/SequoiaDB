
##语法##
***db.collectionspace.collection.attachCL(\<subCLFullName\>, \<options\>)***

在主分区集合下挂载子分区集合。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| subCLFullName | string | 子分区集合名（包含集合空间名）。 | 是 |
| options | Json 对象 |  分区范围，包含两个字段“LowBound”（区间左值）以及“UpBound”（区间右值），例如：{LowBound:{a:0},UpBound:{a:100}}表示取字段“a”的范围区间：[0, 100)。 | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

| 错误码 | 可能的原因     | 解决方法    |
| ------ | -------------- | ----------- |
| -236   | 无效的分区集合 | 检查主分区集合信息是否正确，主分区集合需要设置属性IsMainCL为true。 |
| -23    | 集合不存在     | 检查子分区集合是否存在，如果不存在请创建对应的子分区集合。 |
| -237   | 新增区间与现有区间冲突 |查看现有区间，修改新增区间范围。|

##示例##

* 在主分区集合的指定区间下挂载子分区集合

 ```lang-javascript
 > db.sample.employee.attachCL( "sample2.January", { LowBound: { date: "20130101" }, UpBound: { date: "20130131" } } )
 ```
