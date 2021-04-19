## 名称

getSyncStatus - 获取 STP 节点与当前同步源的同步信息

## 语法

**stp.getSyncStatus()**

## 类别

Stp

## 描述

该函数用于获取 STP 节点与当前同步源的同步状态、同步次数等同步信息。

## 参数

无

## 返回值

函数执行成功时，将通过游标（SdbCursor）返回 STP 节点与当前同步源的同步信息列表，返回的字段信息可参考 [stpq 查询同步信息][stpq]。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

## 版本

v5.0 及以上版本

## 示例

- 获取 STP 节点与当前同步源的同步信息

    ```lang-javascript
    > var stp = new Stp()
    > stp.getSyncStatus()
    {
      "Role": "client",
      "IsPrimary": false,
      "SyncStatus": "CheckSlewRate",
      "SyncSource": {
        "Role": "server",
        "HostName": "server-1",
        "Service": "9622",
        "SyncCount": 59,
        "ValidCount": 41,
        "MinDelay": 250910,
        "MaxDelay": 14284283,
        "InitOffset": 0,
        "NegOffset": {
          "Count": 20,
          "Min": -9898,
          "Max": -3910477
        },
        "PosOffset": {
          "Count": 20,
          "Min": 5731,
          "Max": 3778019
        },
        "LastDelay": 339699,
        "LastOffset": 22779,
        "LastPassed": 1490,
        "SyncHistory": [
          {
            "RequestID": 41,
            "SyncStatus": "CheckOffset",
            "Delay": 5234466,
            "Offset": 1534259,
            "SyncPassed": 70890
          },
          ...
        ]
      }
    }
    ```

- STP server 主节点作为同步源，不需要与任何节点进行同步，所以没有同步状态信息

    ```lang-json
    > stp.getSyncStatus()
    {
      "Role": "server",
      "IsPrimary": true
    }
    ```

[^_^]:
    本文使用的所有引用及链接
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
