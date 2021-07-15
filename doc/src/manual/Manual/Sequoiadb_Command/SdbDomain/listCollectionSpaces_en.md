##NAME##

listCollectionSpaces - enumerate the collection space information in the domain

##SYNOPSIS##

**domain.listCollectionSpaces()**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to enumerate all the collection space information in the specified domain.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of collection details through this object. For field descriptions, refer to [collectionspaces_list][collectionspaces_list].

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get the collection space information under the specified domain.

```lang-javascript
> domain.listCollectionSpaces()
{
    "Name": "sample" 
}
```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[collectionspaces_list]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
