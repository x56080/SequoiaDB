##NAME##

setGroups - reset the replication group contained in the domain

##SYNOPSIS##

**domain.setGroups(\<options\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to reset the replication group contained in the domain.

>**Note:**
>
> User must ensure that the group does not contain any data before deleting a replication group, otherwise the operation will fail.

##PARAMETERS##

options ( *object, required* )

Modify the domain attributes through the options parameters:

-  Groups ( *string/array* ): All replication groups contained in the domain.

   Format: `Groups: ['group1', 'group2']`

-  AutoSplit ( *boolean* ): Whether the domain is automatically split.

   Format: `AutoSplit: true | false`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setGroups()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST |The replication group does not exist | Use the list to check whether the replication group exists. |
| -256   | SDB_DOMAIN_IS_OCCUPIED |The domain has been used  | Use listCollectionSpaces() to check whether there is a collection space in the domain. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Create a domain containing two replication groups, and then turn on automatic split.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
    ```

- Delete a replication group "group2" from the domain and add another replication group "group3". Two replication groups "group1" and "group3" are contained in the domain at last. 

    ```lang-javascript
    > domain.setGroups({Groups: ['group1', 'group3']})
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md