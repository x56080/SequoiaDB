## 名称

start - 启动当前复制组

## 语法

**rg.start()**

## 类别

SdbReplicaGroup

## 描述

启动当前复制组。复制组启动之后才能创建节点及其他操作。也可以使用方法 [db.startRG(< name >))](manual/Manual/Sequoiadb_Command/Sdb/startRG.md) 启动指定的节点。

## 参数

无

## 返回值

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

## 错误

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 版本

v2.0 及以上版本

## 示例

启动 group1 复制组

```lang-javascript
> var rg = db.getRG("group1")
> rg.start()   //等价于 db.startRG("group")
```
