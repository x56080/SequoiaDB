## 名称

truncateLob - 截短集合中的大对象

## 语法

**db.collectionspace.collection.truncateLob\(\<oid\>, \<length\>\)**

## 类别

SdbCollection

## 描述

该函数用于截短集合中的大对象。

## 参数

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| oid    | string | 大对象的唯一描述符。 | 是 |
| length | int | 截短到的长度，必须是大于等于0的值。当length大于等于大对象的大小时，大对象不发生变化。 | 是 |

## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v3.0 及以上版本

## 示例

* 截短一个描述符为'5435e7b69487faa663000897'的大对象的长度到0

 ```lang-javascript
 > db.sample.employee.truncateLob('5435e7b69487faa663000897', 0)
 ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md

