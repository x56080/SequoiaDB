##NAME##

removeGroups - remove the replication group from the domain

##SYNOPSIS##

**domain.removeGroups(\<object\>)**


##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to remove the replication group in the domain.

>**Note:**
>
> User must ensure that the group does not contain any data before removing a replication group, otherwise the operation will fail.

##PARAMETERS##

options ( *object, required* )

Modify the property list through the options parameter:

-  Groups ( *string/array* ): The replication group that the domain will remove.

   Format: `Groups: ['group1', 'group2']`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `removeGroups()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST |Partition group does not exist | Use the list to check whether the partition group exists. |
| -256   |SDB_DOMAIN_IS_OCCUPIED | The domain has been used  | Use domain.listCollectionSpaces() to check whether there is a collection space in the domain. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Create a domain containing three replication groups and turn on automatic split.

```lang-javascript
> var domain = db.createDomain('mydomain', ['group1', 'group2', 'group3'], {AutoSplit: true})
```

- Remove the replication group "group2" from the domain.

    ```lang-javascript
    > domain.removeGroups({Groups: ['group2']})
    ```

- Remove the replication group "group1" and "group2" from the domain.

    ```lang-javascript
    > domain.removeGroups({Groups: ['group1', 'group2']})
    ```  


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md