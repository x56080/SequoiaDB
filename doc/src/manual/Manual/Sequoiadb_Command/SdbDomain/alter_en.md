##NAME##

alter - modify the properties of the domain

##SYNOPSIS##

**domain.alter(\<options\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to modify the properties of the domain.

>**Note:**
>
> - User must ensure that the group does not contain any data before deletiong a replication group, otherwise the operation will fail.
> - Changing "AutoSplit" will not affect the existing collection and collection space.

##PARAMETERS##

options ( *object, required* )

The properties of the domain can be modified through the options parameter:

-  Groups ( *string/array* ): All replication groups contained in the domain.

   Format: `Groups: ['group1', 'group2']`

-  AutoSplit ( *boolean* ): Whether the domain is automatically split.

   Format: `AutoSplit: true|false`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `alter()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST|Partition group does not exist | Use the list to check whether the partition group exists. |
| -214   | SDB_CAT_DOMAIN_NOT_EXIST| Domain does not exist     | Use listDomains() to check whether the domain exists. |
| -256   | SDB_DOMAIN_IS_OCCUPIED |This domain has been used   | Use listCollectionSpaces() to check whether there is a collection space in the domain. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Create a domain containing two replication groups, and then turn on automatic split.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
    ```

- Delete a replication group "group2" from the domain and add another replication group "group3". Two replication group "group1" and "group3" are contained in the domain at last.

    ```lang-javascript
    > domain.alter({Groups: ['group1', 'group3']})
    ```

- First create a domain containint a replication group, and the replication group contains the table "sample.employee".

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

- Delete the original replication group from the domain and add another replication group. Deleting a "group1" with data from the domain will report an error.

    ```lang-javascript
    > domain.alter({Groups: ['group2']})
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md