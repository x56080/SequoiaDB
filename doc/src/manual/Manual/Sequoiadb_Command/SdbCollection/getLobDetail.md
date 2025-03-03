##名称##

getLobDetail - 获取大对象被读写访问的详细信息

##语法##

**db.collectionspace.collection.getLobDetail\(\<oid\>, [detail]\)**

##类别##

SdbCollection

##描述##

该函数用于获取集合中的大对象被读写访问的详细信息。

##参数##

| 参数名    | 参数类型 | 描述   | 是否必填 |
| --------- | -------- | ------ | -------- |
| oid       | string   | 大对象的唯一描述符。              | 是 |
| detail    | boolean  | 位置信息中是否显示详细分片号      | 否 |

  > **Note**
  >
  > 当大对象大小大于2GB时，`Location` 和 `PiecesNum`只会按2GB进行计算。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取大对象被读写访问的详细信息列表。

函数执行失败时，将抛异常并输出错误信息。

大对象被读写访问的详细信息格式为：

|字段名     |描述                  |
| --------- | --------             |
|Oid        |大对象的唯一描述符。  |
|AccessInfo |被读写访问的详细信息。对象类型|
|GroupID    |元数据分片所在的数据组ID。|
|ContextID  |本次操作的上下文标识。|
|Location   |大对象分片位置信息。数组对象类型|
|PiecesNum  |大对象分片数量。      |
|SubCLName  |大对象所在子集合名。（仅当从主表执行操作才显示）|
|Size       |大对象大小。          |
|CreateTime |创建时间。            |
|ModificationTime|修改时间。       |
|Version    |版本号。              |
|Available  |是否有效。            |
|Flag       |状态标识。            |
|HasPiecesInfo|是否有分片空洞。    |
|PiecesInfoNum|元数中分片定义段数量。  |

其中 AccessInfo 的详细信息格式为：

|字段名     |描述                  | 说明 |
| --------- | --------             | ---  |
|RefCount   |大对象当前被引用的总个数。  | RefCount 为 ReadCount, WriteCount, ShareReadCount 之和。|
|ReadCount  |大对象当前被以只读模式打开的个数。| |
|WriteCount |大对象当前被以写模式打开的个数。| 以读写模式打开的计数也计算在此项。|
|ShareReadCount |大对象当前被以共享读模式打开的个数。| getLobRuntimeDetail 命令本身会增加一次 ShareReadCount |
|LockSections |记录大对象中被加锁的区域，以及进行加锁操作的上下文标识。| 可以通过该项查看大对象是被哪些上下文持有锁。S 为读锁；X 为写锁|

其中 Location 数组对象的详细信息格式为：

|字段名     |描述                  | 说明 |
| --------- | --------             | ---  |
|GroupID    | 数据组ID             |      |
|PiecesNum  | 位于该数据组的分片数量|     |
|Pieces     | 分片ID数组           | 仅当 detail=true 时显示|


##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##示例##

列取 00005deb85c5350004743b09 的 lob 当前被访问的详细信息

```lang-javascript
 > db.sample.employee.getLobDetail('00005deb85c5350004743b09')
 {
   "Oid": "00005deb85c5350004743b09",
   "AccessInfo": {
     "RefCount": 3,
     "ReadCount": 0,
     "WriteCount": 1,
     "ShareReadCount": 2,
     "LockSections": [
       {
         "Begin": 10,
         "End": 30,
         "LockType": "X",
         "Contexts": [
           11
         ]
       },
       {
         "Begin": 30,
         "End": 50,
         "LockType": "S",
         "Contexts": [
           12
         ]
       }
     ]
   },
   "GroupID": 1001,
   "ContextID": 14,
   "Location": [
     {
       "GroupID": 1000,
       "PiecesNum": 107
     },
     {
       "GroupID": 1001,
       "PiecesNum": 120
     },
     {
       "GroupID": 1006,
       "PiecesNum": 123
     }
   ],
   "PiecesNum": 350,
   "Size": 91635840,
   "CreateTime": {
     "$timestamp": "2025-02-21-16.33.44.460000"
   },
   "ModificationTime": {
     "$timestamp": "2025-02-21-16.33.46.975000"
   },
   "Version": 2,
   "Available": true,
   "Flag": 0,
   "HasPiecesInfo": false,
   "PiecesInfoNum": 0
 }
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
