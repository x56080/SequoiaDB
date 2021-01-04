##NAME##

dropCS - Delete a specified collection space.

##SYNOPSIS##

***db.dropCS(\<name\>,[options])***

##CATEGORY##

Sdb

##DESCRIPTION##

Delete a specified collection space.

##PARAMETERS##

* `name` ( *String*， *Required* )

    Collection space name.

* `options` ( *Object*， *Optional* )

    The options for dropping the collection space, could be a combination of 
    the following options:

    1. `EnsureEmpty` ( *Bool* ): Ensure the collection space is empty or not, default to be false.

        * true: when collection space contains collection, dropping is canceled and return with error code -275 ; when collection space does not contain any collection, dropping is proceed.
        * false: dropping is proceed whether collection space contains collection or not.

        Format: `EnsureEmpty:true|false`

##RETURN VALUE##

On success, the specified collection space is dropped.

On error, exception will be thrown.

##ERRORS##

The exceptions of `dropCS()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | The collection space is not exist. | Check whether collection space is exist or not. |
| -275 | SDB_DMS_CS_NOT_EMPTY | The collection space is not empty. | Check whether the "EnsureEmpty" option is enabled or not. |

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Drop an exist collection space named by "foo".

    ```lang-javascript
    > db.dropCS("foo")
    Takes 0.003132s.
    ```
