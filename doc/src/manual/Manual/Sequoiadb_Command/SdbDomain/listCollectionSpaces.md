## 名称

listCollectionSpaces - 枚举域中的集合空间信息

## 语法

**domain.listCollectionSpaces()**

## 类别

SdbDomain

## 描述

该函数用于枚举指定域中的全部集合空间信息。

## 参数

无

## 返回值

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看[集合空间列表][collectionspaces_list]。

函数执行失败时，将抛异常并输出错误信息。


## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v2.0 及以上版本

## 示例

* 获取指定域下的集合空间信息

 ```lang-javascript
  > domain.listCollectionSpaces()
  {
      "Name": "sample" 
  }
 ```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[collectionspaces_list]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
