## 名称

getMaster - 获取当前复制组的主节点

## 语法

**rg.getMaster()**

## 类别

SdbReplicaGroup

## 描述

获取当前复制组的主节点。

## 参数

无

## 返回值

返回当前复制组的主节点，类型为 SdbNode 对象。

## 错误

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 版本

v2.0 及以上版本

## 示例

获取 group1 复制组的主节点，可以通过该节点进行相关的节点级操作：

```lang-javascript
> var rg = db.getRG("group1")
> var node = rg.getMaster()
> println(node)
hostname1:11830
> println(node.constructor.name)
SdbNode
> node.help()

   --Instance methods for class "SdbNode":
   connect()                  - Connect the database to the current node.
   getHostName()              - Return the hostname of a node.
   getNodeDetail()            - Return the information of the current node.
   getServiceName()           - Return the server name of a node.
   start()                    - Start the current node.
   stop()                     - Stop the current node.
```

