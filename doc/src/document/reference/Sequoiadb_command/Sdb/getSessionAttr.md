##语法##

***db.getSessionAttr ()***

获取会话属性。

###返回值###

Json 对象，表示会话属性，字段信息如下

| 属性名 | 描述      |
| ------ | --------- |
| PreferedInstance | 会话读操作优先选择的实例，取值列表："M"、"m"、"S"、"s"、"A"、"a"、1-255。可以使用数组指定多个取值。<br>"M", "m"：可读写实例（主实例）<br>"S", "s"：只读实例（备实例）<br>"A", "a"：任意实例<br>1-255：通过 --instanceid 指定实例 ID 的实例 |
| PreferedInstanceMode | 指定会话当多个实例符合 PreferedInstance 的条件时的选择模式。<br>"random"：从候选的实例中随机选择。<br>"ordered"：从候选的实例中按照 PerferedInstance 的顺序进行选择。 |
| Timeout | 指定会话执行操作的超时时间（单位：毫秒），-1 表示不进行超时检测。 |

>   **Note:**
>
>   *   在独立模式或者非协调节点上返回空的 Json 对象。
>   *   设置会话属性请参考 [Sdb.setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md) 。

##示例##

* 获取会话属性

```lang-javascript
> db.getSessionAttr()
```
