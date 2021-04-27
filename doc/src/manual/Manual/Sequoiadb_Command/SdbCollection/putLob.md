## 名称

putLob - 在集合中插入大对象

## 语法

**db.collectionspace.collection.putLob\(\<file path\>, [oid]\)**

## 类别

SdbCollection

## 描述

该函数用于在集合中插入大对象。

## 参数

| 参数名    | 参数类型 | 描述     | 是否必填 |
| --------- | -------- | -------- | -------- |
| file path | string   | 待上传文件的绝对路径，用户需拥有该文件的读权限 | 是 |
| oid       | string   | 指定大对象的 oid | 否 |

## 返回值

函数执行成功时，将返回一个 String 类型的 oid 字符串。用户可通过 oid 对大对象进行相关操作。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v2.0 及以上版本

## 示例

* 创建集合空间与集合

 ```lang-javascript
 > db.createCS('sample' )
 > db.sample.createCL('employee')
 ```

* 上传大对象文件 mylob.txt

 ```lang-javascript
 > db.sample.employee.putLob('/opt/mylob/mylob.txt')
 0000604f989a390002db009e
 Takes 0.010776s.
 ```

* 上传指定 oid 的大对象文件 mylob.txt

 ```lang-javascript
 > db.sample.employee.putLob('/opt/mylob/mylob.txt', '5bf3a024ed9954d596420256')
 5bf3a024ed9954d596420256
 Takes 0.010756s.
 ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
