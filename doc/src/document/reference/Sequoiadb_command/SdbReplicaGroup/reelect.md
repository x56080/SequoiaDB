##语法##
***rg.reelect( [options] )***

在当前复制组中重新选举。

##参数描述##

| 参数名  | 参数类型  | 描述       | 是否必填 |
| ------- | ----------| ---------- | -------- | 
| options | Json 对象 | 可选项，详见如下option选项说明。 | 否       |

##options选项##

| 参数名   | 参数类型 | 描述                        | 默认值 |
| ---------| -------- | --------------------------- | ------ |
| Seconds  | int      | 重新选举需要在多少秒内完成。| 30     |

> **Note:**  
> 1. 返回超时错误代表在规定时间内重选没有完成。通过 db.listReplicaGroups() 观察最终结果。  
> 2. 只有复制组中存在主节点时才可以进行重新选举。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息，或通过 [getLastError](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。
关于错误处理可以参考 [常见错误处理指南](troubleshooting/general/general_guide.md) 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

在 group1 中进行重新选举，超时时间为60s。

```lang-javascript
> var rg = db.getRG("group1")
> rg.reelect({Seconds:60})
```
