##NAME##

invalidateCache - clear the cache of the nodes

##SYNOPSIS##

**db.invalidateCache( [options] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to clear the cache of the catalog, data or coord nodes.

##PARAMETERS##

- options ( *object, optional* )

    - [command positional parameter][location]: Specify the positional parameters for command execution, such as host, node, etc. Default is clearing the cache of the catalog, data and coord nodes.

        Format: `Role: "Coord"` or `GroupName: ["group1", "group2"]`

    - Type ( *string* ): The cache type, such as: `"catalog"`, `"group"`, `datasource` and `strategy`. Default is clearing all types of cache.

        Format: `Type: "catalog"`

    - Name ( *string* or *array* ): The specific object corresponding to each type of cache, such as the "catalog" cache for a specific collection.

        Format: `Name: "cs.cl"`  or `Name: ["cs.cl", "foo.bar"]`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Clear the cache of the current coord node.

```lang-javascript
> db.invalidateCache( { Global: false } )
```

* Clear the cache of the current coord node and 'group1' group's nodes.

```lang-javascript
> db.invalidateCache( { GroupName: 'group1' } )
```



[^_^]:
    links
[location]:manul/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md