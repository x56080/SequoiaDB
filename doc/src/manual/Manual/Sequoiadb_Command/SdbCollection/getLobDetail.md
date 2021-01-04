
## 语法##
***db.collectionspace.collection.getLobDetail\(\<oid\>\)***

获取集合中的大对象被读写访问的详细信息。

##参数描述##

| 参数名    | 参数类型 | 描述   | 是否必填 |
| --------- | -------- | ------ | -------- |
| oid       | string   | 大对象的唯一描述符。              | 是 |

##返回值##

返回大对象被读写访问的详细信息。出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

大对象被读写访问的详细信息格式为：

|字段名     |描述                  |
| --------- | --------             |
|Oid        |大对象的唯一描述符。  |
|AccessInfo |被读写访问的详细信息。|
|ContextID  |本次操作的上下文标识。|

其中 AccessInfo 的详细信息格式为：

|字段名     |描述                  | 说明 |
| --------- | --------             | ---  |
|RefCount   |大对象当前被引用的总个数。  | RefCount 为 ReadCount, WriteCount, ShareReadCount 之和。|
|ReadCount  |大对象当前被以只读模式打开的个数。| |
|WriteCount |大对象当前被以写模式打开的个数。| 以读写模式打开的计数也计算在此项。|
|ShareReadCount |大对象当前被以共享读模式打开的个数。| getLobRuntimeDetail 命令本身会增加一次 ShareReadCount |
|LockSections |记录大对象中被加锁的区域，以及进行加锁操作的上下文标识。| 可以通过该项查看大对象是被哪些上下文持有锁。S 为读锁；X 为写锁|



##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 列取 00005deb85c5350004743b09 的 lob 当前被访问的详细信息

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
      "ContextID": 14
    }
   ```

