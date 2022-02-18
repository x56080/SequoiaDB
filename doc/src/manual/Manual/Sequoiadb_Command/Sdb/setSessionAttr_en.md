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

The attributes of the session can be set through the options parameter:

- PreferredInstance ( *string/array/number* ): Preferences for session read operations. The defaul is the value of the parameter "preferdinstance" in the coord node configuration file. If the "preferredinstance" is not configured, the default is "M".

    The value type is divided into role value and instance value. Users can use an array to specify multiple values, details as follows:

    Role value:

    Users can use an array to specify multiple values, and the specific values are as follows:
    - "M", "m": Read and write instance(primary instance).
    - "S", "s": Read only instance(secondary instance).
    - "A", "a": Any instance.
    - 1~255: Specify the instance ID by instanceid.

    Instance value:

    - 1~255:[instance ID][instance].

    >**Note:**
    >
    > - When specifying the value of the instance and the role at the same time, the standby node that matches the instance ID of 1 is preferred. If the corresponding node is not matched, the node in the data group will be randomly selected.For example, when the value is [1, "S"], it means that the standby node with instance ID 1 is selected first.
    > - The symbol "-" is used to extend the value of the role. If users use "-" to expend the role value, if the corresponding node is not matched, the node in the specified role will be randomly selected. For example, when the value is [1, "-S"], if the corresponding node is not matched, the node will be randomly selected from the standby nodes included in the daa group.
    > - When multiple role values are specified, the semantics of the role and its expanded value are the same, and only the first value takes effect. For example, when the value is ["-M", "S"], only "-M" takes effect, indicating that the standby node in the data group is selected first.
    > - When specifying an instance value independently, if there is no node corresponding to the instance ID in the data group, the instanceid of the node will be reassigned from 1 according to the positive sequence of the NodeID in the data group. At the same time, the server will convert the instance ID according to (instanceid - 1)%(total number of nodes), and then match the converted value with the instanceid to obtain the corresponding node.
    > - If there is a write request before the read request in the same session, the read request will use the node (read-write instance) used by the write request for reading by default within the validity period. Users can modify the validity period of the read request multiplexed write request node by configuring "PreferredPeriod".

   Format: `PreferredInstance: "M"` or `PreferredInstance: [1, 10, "S"]`

- PreferredInstanceMode ( *string* ): When there are multiple candidate instances, specify the session selection mode. The default is the value of the parameter "preferredinstancemode" in the coord node configuration file. If there is no configuration "preferredinstancemode", the default is "random".

- PreferedInstanceMode ( *string* ): Specify the selection mode of the session when multiple instances meet the conditions of PreferedInstance.

    The parameter values are as follows:
    - "random": Randomly select from candidate instance values.
    - "ordered"：Select from the candidate instance values in the order of PerferedInstance.
    
    The value of the role in parameter "PreferedInstance" is better than or inferior to the value of the instance when selected according to the rules, and has nothing to do with the value of parameter "PreferedInstanceMode".

    Format: `PreferredInstaceMode: "random"`

- PreferredPeriod ( *number* ): Specify the effective period of the preferred instance in seconds. The default is the value of the parameter "preferredperiod" in the coord node configuration file. If the "preferredperiod" is not configured, the default is 60.

    - After each "PreferredPeriod" valid period, the read request will will re-select the appropriate instance for query according to the value of "PreferredInstance".
    - The value range of this parameter is [-1, 2^31-1]; a value of -1 means no invalidation; a value of 0 means that the last selected instance is not used for this query and reselected according to parameter "Preferredinstance".
    - This parameter is only applicable to SequoiaDB v2.8.9, v3.2.5 and above.

    Format: `PreferredPeriod: 60`

- PreferredStrict ( *boolean* ): Specifies whether the node selection is strict mode. The default value is false, non-strict mode.

    When specified as strict mode, the node can only be selected from the instance value specified by the parameter "Preferredinstance". if "Preferredinstance" does not specify an instance value, this parameter does not take effect.

    Format: `PreferredStrict: true`

- Timeout ( *number* ): Specify the timeout period for the session to perform operations, in milliseconds.

    The minimum value of this parameter is 1000 milliseconds, and a value of -1 means no timeout detection is performed.

    Format: `Timeout: 10000`

- TransIsolation ( *number* ): Isolation level of session transaction.

    The parameter values are as follows:
    - 0: RU level
    - 1: RR level
    - 2: RS level.

    Format: `TransIsolation: 1`

- TransTimeout ( *number* ): Session transaction lock wait timeout time, in seconds.

    Format: `TransTimeout: 10`

- TransLockWait ( *boolean* ): Whether the session transaction need to wait for locks under and the RC isolation level.

    Format: `TransLockWait: true`

- TransUseRBS ( *boolean* ): Whether the session transcation uses the rollback segment.

    Format: `TransUseRBS: true`

- TransAutoCommit ( *boolean* ): Whether the session transaction supports the automatic transaction commit.

    Format: `TransAutoCommit: true`

- TransAutoRollback ( *boolean* ): Whether the session transaction is automatically rolled back when the operation fails.

    Format: `TransAutoRollback: true`

- TransRCCount ( *boolean* ): Whether the session transaction uses read committed to process count() queries.

    Format: `TransRCCount: true`

> **Note:**
>
> * The default values of "PreferedInstance" and "PreferedInstaceMode" are the values of "preferedinstance" and "preferedinstancemode" in the coordination node configuration.
>    * The default value of the coordination node configuration "preferedinstance" is "M", and the default value of "preferedinstacemode" is "random".
> * The value of "PreferedInstance" is divided into two categories, one is the role value, such as "M", "S", etc. The other is the instance value. That is, the instance ID set by the data node through the configuration "instanceid".
>     1. Role value: "M", "m": read-write instance(primary instance); "S", "s": read-only instance(secondary instance); "A", "a": any instance.
>     2. Instance value: 1~255, specify the node that matches the setting of "instanceid". Data nodes can be used in conjunction with configuration "instanceid".
>         * The ID of the instance can be configurad through the configuration item "instanceid" of the data node. Multiple data nodes with the same instance ID can be configured in the same data group.
>         * The configuration item "instanceid" of modifying data node cannot take effect dynamically, and the data node needs to be stopped and started manually. After restarting, users also need to manually call [Sdb.invalidateCache()][invalidateCache] to clear the cache of each coordination node.
>         * When the node is configured with "instanceid", obtain it accroding to "instanceid". In the case that the node is not configured with "instanceid", select "instanceid" accroding to the sequence of the NodeID of the node in the group(starting from 1). For example, there are three nodes   [ {NodeID:1001}, {NodeID:1004}, {NodeID:1002} ] in the group "db1", then the "instanceid" of the nodes are1, 3, 2.
>     3. If the value of one or more instances and the value of a role are mixed and specified, the node that matches the role among a group of nodes matching the instance ID is preferentially selected. For example, ```[ 1, 2, "S" ]``` indicates that the secondary node among the nodes with the instance ID of 1 to 2 is preferentially selected.
>     4. For mixed designation, the role value can be expanded by "-", such as "-M"，"-S" etc., which means that if there is no node matching the instance ID, the role node is preferentially selected. For example, ```[ 1, 2, "-S" ]``` means that the node with the instance ID 1 to 2 is preferentially matched. If not, select any secondary node first.
>     5. When individually specified, the expansion mode of the role value is the same as the semantics of the role value. For example when specify "S" abd "-S" separately, before a read request the samantics are the same.
>     6. If there is a write request before the read request in the same session, for a period of the time after the write request, the read request will be read by default using the same node(read-write instance) as the write request. Users can modify the expiration date of the read request to reuse the write request node by setting "PreferedPeriod". Validity period of request node with write.
>     7. If there is no instance that meets "PreferedInstance", and there is no write request before, the node is generally selected randomly in the data group. For special cases, in order to be compatible with the previous version. If an instance value is specified separately, the total number of nodes will be modularized the instance ID-1 and then selected in the group in ascending order of "NodeID".
> * The default value of "PreferedPeriod" is the value of "preferedperiod" in the coordination node configuration.
>     * If the node that selected the request last time is within the valid period, the read request still uses this node for query. After the cycle, it will be re-selected according to "PreferedInstance".
>     * The default value is 60.
>     * The value rang is [-1, 2^31 - 1].	
>     * -1 means no failure.
>     * 0 means that this query does not use the priority instance seleted last time, and re-select accroding to "PreferedInstance".
>     * This parameter is only applicable to SequoiaDB 2.8.9 version, 3.2.5 and above.
> * The default value of "Timeout" is -1. That is, no timeout detection is performed.
> * Transaction-related properties only allow "TransTimeout" to be set in transactions, and other transaction properties need to be set in non-transactions.
> * Get session attributes refers to [Sdb.getSessionAttr()][getSessionAttr].

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `setSessionAttr()` function are as follows:

| Error Code | Error Type      | Description       | Solution                   |
|--------|----------------|----------------------|----------------------------|
| -6     | SDB_INVALIDARG | The "options" input error | Check the value, range of the set property and etc. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

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



[^_^]:
   links
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[getSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
