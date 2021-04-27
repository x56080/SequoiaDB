## 名称

getLob - 读取大对象

## 语法

**db.collectionspace.collection.getLob\(\<oid\>,\<file path\>,\[forced\]\)**

## 类别

SdbCollection

## 描述

该函数用于读取集合中的大对象。

## 参数

| 参数名    | 参数类型 | 描述   | 是否必填 |
| --------- | -------- | ------ | -------- |
| oid       | string   | 大对象的唯一描述符。              | 是 |
| file path | string   | 待写入的本地文件全路径。          | 是 |
| forced    | bool     | 本地文件如果已经存在是否强制覆盖。| 否 |

> **Note:**
>
> * 本地文件不需要事先手工创建。
> * forced 默认为 false。

## 返回值

函数执行成功时，将返回一个 String 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v2.0 及以上版本

## 示例

* 将标示符为 5435e7b69487faa663000897 的 lob 写入本地 /opt/newlob 文件

 ```lang-javascript
 > db.sample.employee.getLob( '5435e7b69487faa663000897', '/opt/newlob' )
 ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
