[^_^]:
   restoreToTime()

##名称##

restoreToTime - 将集群恢复至指定的时间点

##语法##

**db.restoreToTime([options])**

##类别##

Sdb

##描述##

该函数用于将已开启恢复模式的集群恢复至指定时间点，在此时间点后提交的所有数据更改将被回滚，同时未提交的事务将被终止。

##参数##

options（ *object，必填* ）

设置需要恢复的时间点，可使用的选项如下：

- Time（ *number/string/Timestamp* ）：指定恢复的目标时间点，单位为秒

    该参数取值为 0 时，将恢复至最新的一致性时间点；取值为字符串时，填入值应符合 ISO 8601 格式。

    格式：`Time:0` 或 `Time:1609430400` 或 `Time:"2021-01-01T00:00:00+08:00"` 或 `Time:Timestamp("2021-01-01T00:00:00+08:00")`
    
> **Note:**
>
> 在基于 Unix 的机器中，用户可以使用 `date` 获取或格式化时间。
>
> - 获取符合 ISO 8601 格式的当前时间
>
>    ```lang-bash
>    $ date -Iseconds 
>    2021-01-01T00:00:00+08:00
>    ```
>
> - 将日期格式化为 ISO 8601 格式

>
>    ```lang-bash
>    $ date -Iseconds --date='2021/01/01 15:11:09'
>    2021-01-01T15:11:09+08:00
>    ```
>
> - 获取当前的时间戳
>
>    ```lang-bash
>    $ date +%s
>    1609430400
>    ```

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`restoreToTime()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
|---|---|---|---|
| -359 | SDB_RESTORE_NOT_IN_PROGRESS | 集群未开启恢复模式 | 执行 `db.restorePrepare()` 命令开启集群的还原模式 |
| -360 | SDB_RESTORE_NO_CONSISTENT_PIT | 没有有效的一致性时间点或无法恢复至指定时间点 | 还原包含指定时间点的备份 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.2 及以上版本

##示例##

- 恢复至最新的一致性时间点

    ```lang-javascript
    > db.restoreToTime({Time: 0})
    ```

- 使用时间戳恢复至指定时间点

    ```lang-javascript
    > db.restoreToTime({Time: 1577836800})
    ```

- 使用 ISO 8601 字符串恢复至指定时间点

    ```lang-javascript
    > db.restoreToTime({Time: "2020-01-01T00:00:00+00:00"})
    ```

- 使用 Timestamp 恢复至指定时间点

    ```lang-javascript
    > db.restoreToTime({Time: Timestamp("2020-01-01T00:00:00+00:00")})
    ```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md

