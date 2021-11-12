##NAME##

alter - modify the properties of the domain

##SYNOPSIS##

**domain.alter(\<options\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to modify the properties of the domain.

##PARAMETERS##

options ( *object, required* )

Modify the properties of the domain through the parameter "options":

-  Groups ( *string/array* ): The replication group contained in the domain.

    When modifying the replication group contained in the domain, if it involves deleting the replication group, make sure that there is no data in the replication group, otherwise the operation will report an error.

    Format: `Groups: ['group1', 'group2']`

-  AutoSplit ( *boolean* ): Whether to enable automatic segmentation, the default value is false, not enabled.

    After this parameter is modified, it will only take effect for the newly created collection space and collection.

    Format: `AutoSplit: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `alter()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST|Partition group does not exist. | Use the list to check whether the partition group exists. |
| -214   | SDB_CAT_DOMAIN_NOT_EXIST| Domain does not exist.     | Use [listDomains()](reference/Sequoiadb_command/Sdb/listDomains.md) to check whether the domain exists. |
| -256   | SDB_DOMAIN_IS_OCCUPIED |Domain has been used.   | Use [listCollectionSpaces()](reference/Sequoiadb_command/Sdb/listCollectionSpaces.md) to check whether there is a collection space in the domain. |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

- Create a domain containing the replication group "group1" and "group2", no data exists in the replication groups.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'])
    ```

    Modify the replication group contained in the domain to "group1" and group3.

    ```lang-javascript
    > domain.alter({Groups: ['group1', 'group3']})
    ```

- Create a domain containing the replication group "group1" with data in the replication group.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

    Modify the replication group contained in the domain to "group2".
 
    ```lang-javascript
    > domain.alter({Groups: ['group2']})
    ```

    Because there is data in "group1", the operation reports an error.
   
    ```lang-javascript
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```