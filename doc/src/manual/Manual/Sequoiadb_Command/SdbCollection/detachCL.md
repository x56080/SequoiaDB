
## 语法##
***db.collectionspace.collection.detachCL\(\<subCLFullName\>\)***

从主分区集合中分离出子分区集合。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| partitionName | string | 子分区名（原子分区集合名） | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 从主分区集合中分离指定子分区

 ```lang-javascript
 > db.sample.year.detachCL("sample2.January")
 ```
