
## 语法##
***db.collectionspace.collection.getLob\(\<oid\>,\<file path\>,\[forced\]\)***

读取集合中的大对象。

##参数描述##

| 参数名    | 参数类型 | 描述   | 是否必填 |
| --------- | -------- | ------ | -------- |
| oid       | string   | 大对象的唯一描述符。              | 是 |
| file path | string   | 待写入的本地文件全路径。          | 是 |
| forced    | bool     | 本地文件如果已经存在是否强制覆盖。| 否 |

> **Note:**
>
> * 本地文件不需要事先手工创建。
> * forced 默认为 false。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 将标示符为 5435e7b69487faa663000897 的 lob 写入本地 /opt/newlob 文件

 ```lang-javascript
 > db.sample.employee.getLob( '5435e7b69487faa663000897', '/opt/newlob' )
 ```
