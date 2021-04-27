## 名称

truncate - 删除集合内所有数据

## 语法

**db.collectionspace.collection.truncate\(\)**

## 类别

SdbCollection

## 描述

该函数用于删除集合内所有数据（包括普通文档和 LOB 数据），但不会影响其元数据。与 remove 需要按照条件筛选目标不同，truncate 会直接释放数据页，在清空集合（尤其是大数据量下）数据时效率比 remove 更加高效。

> **Note:** 
> 
> 如有自增字段，truncate后字段序列值将会重置。

## 参数

无

## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v2.0 及以上版本

## 示例

* 我们在集合 sample.employee 中插入了普通数据和 LOB 数据。通过快照查看其数据页使用情况：

 ```lang-javascript
 > db.snapshot(SDB_SNAP_COLLECTIONS) ;
 {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 10000,
              "TotalDataPages": 33,
              "TotalIndexPages": 7,
              "TotalLobPages": 36,
              "TotalDataFreeSpace": 41500,
              "TotalIndexFreeSpace": 103090
            }
          ]
        }
      ]
    }
 ```

* 上例中可以看到其中数据页为33，索引页为7，LOB 页为36。下面执行 truncate 操作。

 ```lang-javascript
 > db.sample.employee.truncate()
 ```

* 再次通过快照查看数据页使用情况，可以查看除索引页为2（存储了索引的元数据信息）外，其余数据页已经全部被释放了。

 ```lang-javascript
 > db.snapshot(SDB_SNAP_COLLECTIONS) ;
 {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 0,
              "TotalDataPages": 0,
              "TotalIndexPages": 2,
              "TotalLobPages": 0,
              "TotalDataFreeSpace": 0,
              "TotalIndexFreeSpace": 65515
            }
          ]
        }
      ]
    }
 ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md

