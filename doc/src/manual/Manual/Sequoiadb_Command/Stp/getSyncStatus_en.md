##NAME##

getSyncStatus - get the synchronization information between the STP node and the current synchronization source

##SYNOPSIS##

**stp.getSyncStatus()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the synchronization information, such as the synchronization status and the number of synchronizations between the STP node and the current synchronization source.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the synchronization information list of STP node and current synchronization source through the cursor. Users can refer to [stpq query synchronization information][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

- Get the synchronization information between the STP node and the current synchronization source.

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

- The STP server master node is used as the synchronization source and does not need to be synchronized with any node, all without synchronization status infromation.

    ```lang-json
    > stp.getSyncStatus()
    {
      "Role": "server",
      "IsPrimary": true
    }
    ```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
