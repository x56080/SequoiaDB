[^_^]:
     restoreCheck()

##名称##

restoreCheck - 检查是否可恢复至指定时间点

##语法##

**db.restoreCheck([options])**

##类别##

Sdb

##描述##

SequoiaDB 集群会根据当前各数据节点的日志和可用日志空间，计算可以进行全局恢复的一致性时间窗口。该函数用于检查指定的时间点是否在当前时间窗口内，或检查集群是否可以恢复至指定的时间点。同时，可用于获取最新的一致性时间点。

##参数##

options（ *object，必填* ）

设置需要检查的时间点，可使用的选项如下：

- Time（ *number/string/Timestamp* ）：指定需要检查的目标时间点，单位为秒

    该参数取值为 0 时，将获取最新的一致性时间点；取值为字符串时，填入值应符合 ISO 8601 格式。

    格式：`Time: 0` 或 `Time: 1609430400` 或 `Time: "2021-01-01T00:00:00+08:00"` 或 `Time: Timestamp("2021-01-01T00:00:00+08:00")`

> **Note:**
>
> 在基于 Unix 的机器中，用户可以使用 `date` 获取或格式化时间。
>
> - 获取符合 ISO 8601 格式的当前时间
>
>     ```lang-bash
>     $ date -Iseconds
>     2021-01-01T00:00:00+08:00
>     ```
>
> - 将日期格式化为 ISO 8601 格式
>
>     ```lang-bash
>     $ date -Iseconds --date='2021/01/01 15:11:09'
>     2021-01-01T15:11:09+08:00
>     ```
>
> - 获取当前的时间戳
>
>     ```lang-bash
>     $ date +%s
>     1609430400
>     ```

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取一致性时间点列表，字段说明如下：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| Time   | string | 可用于恢复的一致性时间点 |
| MinRecoverableTime | string | 当前使用的同步日志中所记录的最早可恢复一致性时间点 |
| MaxRecoverableTime | string | 如果集群通过执行 [sdbrestore][restore] 进入恢复模式，该字段值为备份中所记录的最新可恢复一致性时间点；如果集群通过执行 [restorePrepare()][prepare] 进入恢复模式，该字段值为执行 restorePrepare() 的时间 |

函数执行失败时，将抛出异常并输出错误信息。

##错误##

`restoreCheck()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
|---|---|---|---|
| -6   | SDB_INVALIDARG | 所指定的时间点超出当前日志所记录的时间范围 | 所指定的一致性时间点取值范围为[MinRecoverableTime,MaxRecoverableTime] |
| -359 | SDB_RESTORE_NOT_IN_PROGRESS | 集群未开启恢复模式 | 执行 `db.restorePrepare()` 命令开启集群的还原模式 |
| -360 | SDB_RESTORE_NO_CONSISTENT_PIT | 没有有效的一致性时间点或无法恢复至指定时间点 | 还原包含指定时间点的备份 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.2 及以上版本

##示例##

- 查看最新可恢复的一致性时间点

    ```lang-javascript
    > db.restoreCheck({Time: 0})
    ```

    输出结果如下：

    ```lang-json
    {
     Time: "2020-01-01T12:00:00+00:00",
     MaxRecoverableTime: "2020-01-01T12:00:00+00:00",
     MinRecoverableTime: "2020-01-01T00:00:00+00:00",
    }
    ```

- 检查指定时间点是否为可恢复的一致性时间点

    ```lang-javascript
    > db.restoreCheck({Time: "2020-01-01T03:00:00+00:00"})
    ```

    输出结果如下：

    ```lang-json
    {
     Time: "2020-01-01T03:00:00+00:00",
     MaxRecoverableTime: "2020-01-01T12:00:00+00:00",
     MinRecoverableTime: "2020-01-01T00:00:00+00:00",
    }
    ```

- 如果集群中某一复制组的同步日志没有足够的空间可以写入，即使指定的时间点为可恢复的一致性时间点，也不进行恢复

    ```lang-javascript
    > db.restoreCheck({Time:"2021-03-19-12.49.40.012277"})
    ```

    输出结果如下：

    ```lang-json
    (shell):1 uncaught exception: -203
    No writable log space:
    Restore cannot reach 1620338773126102, limited to 1620338780935745
    ```

    获取详细误信息

    ```lang-javascript
    > getLastErrObj()
    {
     "errno": -203,
     "description": "No writable log space",
     "detail": "Restore cannot reach 1620338773126102, limited to 1620338780935745",
     "ErrNodes": [
       {
         "NodeName": "sdbserver:20000",
         "GroupName": "db1",
         "Flag": -203,
         "ErrInfo": {
           "errno": -203,
           "description": "No writable log space",
           "detail": "Restore cannot reach 1620338773126102, limited to 1620338780935745"
         }
       }
     ]
    }
    ```

- 如果指定的时间点在有效时间窗口之前，则需要历史的备份才能恢复至指定时间点

    ```lang-javascript
    > db.restoreCheck({Time: "2021-03-19-12.49.40.012277"})
    ```

    输出结果如下：

    ```lang-json
    (shell):1 uncaught exception: -6
    Failed restore check
    ```

    获取详细误信息

    ```lang-json
    > getLastErrObj()
    {
     "Detail": "Some nodes require earlier backups to reach the target time",
     "Time": "2021-03-19-12.49.40.012277",
     "MaxRecoverableTime": "2021-03-19-12.49.46.630301",
     "ErrNodes": [
       {
         "NodeName": "sdbserver:20000",
         "GroupName": "db1",
         "MinRecoverableTime": "2021-03-19-12.49.41.012277",
         "MaxRecoverableTime": "2021-03-19-12.49.46.630301"
       }
     ]
    }
    ```

- 如果指定的时间点在有效的时间窗口之后，则需要更新的备份才能恢复至指定时间点

    ```lang-javascript
    > db.restoreCheck({Time: "2021-03-19-12.50.46.630301"})
    ```

    输出结果如下：

    ```lang-json
    (shell):1 uncaught exception: -6
    Failed restore check
    ```

    获取详细错误信息

    ```lang-json
    > getLastErrObj()
    {
      "Detail": "One or more nodes require later backups to reach the target time",
      "Time": "2021-03-19-12.50.46.630301",
      "ErrNodes": [
        {
          "NodeName": "sdbserver:20000",
          "GroupName": "db1",
          "MinRecoverableTime": "2021-03-19-12.49.41.012277",
          "MaxRecoverableTime": "2021-03-19-12.49.46.630301"
        }
      ]
    }
    ```

- 当节点使用不同的备份进行恢复、节点间日志差距太大、节点的日志空间不够或节点上有不可恢复的操作（比如 DDL 操作）时，都有可能造成集群无法找到有效的的时间窗口

    ```lang-javascript
    > db.restoreCheck({Time: 0})
    ```

    输出结果如下：

    ```lang-json
    (shell):1 uncaught exception: -360
    Failed restore check
    ```

    获取详细错误信息

    ```lang-json
    > getLastErrObj()
    {
      "Detail": "No available global consistency point",
      "Time": "1970-01-01-08.00.00.000000",
      "MaxRecoverableTime": "2021-03-19-12.52.01.630926",
      "ErrNodes": [
        {
          "NodeName": "sdbserver:20200",
          "GroupName": "db3",
          "MinRecoverableTime": "2021-03-19-12.52.01.630927",
          "MaxRecoverableTime": "2021-03-19-12.52.01.630926"
        }
      ]
    }
    ```


[^_^]:
    本文使用的所有引用及链接
[restore]:manual/Distributed_Engine/Maintainance/Backup_Recovery/point_in_time_restore.md
[prepare]:manual/Manual/Sequoiadb_Command/Sdb/restorePrepare.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md