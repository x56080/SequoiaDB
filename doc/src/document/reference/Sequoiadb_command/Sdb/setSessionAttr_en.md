##NAME##

setSessionAttr - set session attributes

##SYNOPSIS##

**db.setSessionAttr(\<options\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to set session attributes.

##PARAMETERS##

options ( *object, required* )

The attributes of the session can be set through the parameter "options":

- PreferredInstance ( *string/array/number* ): Preferences for session read operations. The defaul is the value of the parameter "preferdinstance" in the coord node configuration file. If the "perferedinstance" is not configured, the default is "M".

    The value type is divided into role value and instance value. Users can use an array to specify multiple values, details as follows:

    Role value:

    - "M", "m": Read and write instance(primary instance).
    - "S", "s": Read only instance(secondary instance).
    - "A", "a": Any instance.

    Instance value:

    - 1~255:[instance ID](data_model/instance.md).

    >**Note:**
    >
    > - When specifying the value of the instance and the role at the same time, the standby node that matches the instance ID of 1 is preferred. If the corresponding node is not matched, the node in the data group will be randomly selected.For example, when the value is [1, "S"], it means that the standby node with instance ID 1 is selected first.
    > - The symbol "-" is used to extend the value of the role. If users use "-" to expend the role value, if the corresponding node is not matched, the node in the specified role will be randomly selected. For example, when the value is [1, "-S"], if the corresponding node is not matched, the node will be randomly selected from the standby nodes included in the daa group.
    > - When multiple role values are specified, the semantics of the role and its expanded value are the same, and only the first value takes effect. For example, when the value is ["-M", "S"], only "-M" takes effect, indicating that the standby node in the data group is selected first.
    > - When specifying an instance value independently, if there is no node corresponding to the instance ID in the data group, the instanceid of the node will be reassigned from 1 according to the positive sequence of the NodeID in the data group. At the same time, the server will convert the instance ID according to (instanceid - 1)%(total number of nodes), and then match the converted value with the instanceid to obtain the corresponding node.
    > - If there is a write request before the read request in the same session, the read request will use the node (read-write instance) used by the write request for reading by default within the validity period. Users can modify the validity period of the read request multiplexed write request node by configuring "PreferredPeriod".

   Format: `PreferredInstance: "M"` or `PreferredInstance: [1, 10, "S"]`

- PreferredInstanceMode ( *string* ): When there are multiple candidate instances, specify the session selection mode. The default is the value of the parameter "preferredinstancemode" in the coord node configuration file. If there is no configuration "preferredinstancemode", the default is "random".

    The values are as follows:

    - "random": Randomly select from candidate instance values.
    - "ordered"：Select from the candidate instance values in the order of parameter "PerferedInstance".

    Format: `PreferredInstaceMode: "random"`

- PreferredPeriod ( *number* ): Specify the effective period of the preferred instance in seconds. The default is the value of the parameter "preferredperiod" in the coord node configuration file. If the "preferredperiod" is not configured, the default is 60.

    - After each "PreferredPeriod" valid period, the read request will will re-select the appropriate instance for query according to the value of "PreferredInstance".
    - The value range of this parameter is [-1, 2^31-1]; a value of -1 means no invalidation; a value of 0 means that the last selected instance is not used for this query and reselected according to parameter "Preferredinstance".
    - This parameter is only applicable to SequoiaDB v2.8.9, v3.2.5 and above.

    Format: `PreferredPeriod: 60`

- PreferredStrict ( *boolean* ): Specifies whether the node selection is strict mode. The default value is false, non-strict mode.

    When specified as strict mode, the node can only be selected from the instance value specified by the parameter "Preferredinstance". if "Preferredinstance" does not specify an instance value, this parameter does not take effect.

    Format: `PreferredStrict: true`

- Timeout ( *number* ): Specify the timeout period for the session to perform operations, error message will be returned when timeout, in milliseconds, the default value is -1.

    The minimum value of this parameter is 1000 milliseconds, and a value of -1 means no timeout detection is performed.

    Format: `Timeout: 10000`

- TransIsolation ( *number* ): Isolation level of session transaction, the default value is 0.

    The values are as follows:

    - 0: RU level
    - 1: RC level
    - 2: RS level

    Format: `TransIsolation: 1`

- TransTimeout ( *number* ): Session transaction lock wait timeout time, error message will be returned when timeout, in seconds, the default value is 60.

    Format: `TransTimeout: 10`

- TransLockWait ( *boolean* ): Whether the session transaction need to wait for locks under and the RC isolation level, the default value is true, wait for record lock.

    Format: `TransLockWait: true`

- TransUseRBS ( *boolean* ): Whether the session transcation uses the rollback segment, the default value is true, use rollback segment.

    Format: `TransUseRBS: true`

- TransAutoCommit ( *boolean* ): Whether the session transaction supports the automatic transaction commit, the default value is false, not open.

    Format: `TransAutoCommit: true`

- TransAutoRollback ( *boolean* ): Whether the session transaction is automatically rolled back when the operation fails, the default value is true, automatic rollback.

    Format: `TransAutoRollback: true`

- TransRCCount ( *boolean* ): Whether the session transaction uses read committed to process count() queries, the default value is false, use read submitted.

    Format: `TransRCCount: true`

- TransMaxLockNum ( *number* ): Maximum number of record locks can be hold by a transaction on a data node, default value is 10000, range is [ -1, 2^31 - 1 ].

    When the value of this parameter is -1, it means that the transaction has no limit on the number of record locks. When the value is 0, it means that the transaction does not use record locks, but directly uses set locks.

    Format: `TransMaxLockNum: 10000`

- TransAllowLockEscalation ( *boolean* ): Whether to allow lock escalation after the number of record locks held by a transaction exceeds the parameter "transmaxlocknum". The default value is true, which means lock escalation ia allowed.

    If the number of record locks held by the transaction reaches the upper limit, but the value of this parameter is false, the transaction operation will report an error.

    Format: `TransAllowLockEscalation: true`

- TransMaxLogSpaceRatio ( *number* ): Maximum ratio of log space can be used by a transaction on a data node, default value is 50, range is [1, 50].

    This parameter indicates the maximum percentage of transactions in the total log space of the data node(total log space size=logfilesz*logfilenum). When the log space used by the transaction reaches the upper limit, the transaction operation will report an error.

    Format: `TransMaxLogSpaceRatio: 50`

    > **Note：**
    > 
    > - Transaction-related properties only allow "TransTimeout" to be set in transactions, and other transaction properties need to be set in non-transactions.
    > - Get session attributes refers to [getSessionAttr()](reference/Sequoiadb_command/Sdb/getSessionAttr.md).

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `setSessionAttr()` function are as follows:

| Error Code | Error Type      | Description       | Solution                   |
|--------|----------------|----------------------|----------------------------|
| -6     | SDB_INVALIDARG | The "options" input error | Check the value, range of the set property and etc. |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

* Set session priority to get data from the "primary" database instance first.

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: "M"})
    ```

* Set the session to read data from the secondary intance 1 and 3 first.

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: [1, 3, "S"]})
    ```

* Set the operation timeout for the sessions to 10 seconds.

    ```lang-javascript
    > db.setSessionAttr({Timeout: 10000})
    ```
