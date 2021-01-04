
##NAME##

find - Find a file.

##SYNOPSIS##

***File.find( \<options\>, \[filter\] )***

##CATEGORY##

File

##DESCRIPTION##

Find a file.

##PARAMETERS##

| Name    | Type     | Default | Description                | Required or not |
| ------- | -------- | ------- | -------------------------- | --------------- |
| options | JSON     | ---     | find mode and find content | yes             |
| filter  | JSON     | Default to display all content | Filtered conditions | not |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Description | Required or not |
| ---------- | ---- | ----------- | --------------- |
| mode       | char | if { mode: 'n' }, then it means that find files based on file name<br>if { mode: 'u' }, then it means that find files based on username<br>if { mode: 'g' }, then it means that find find based on group name<br>if { mode: 'p' }, then it means that find files based on permission | yes |
| pathname   | string | specify the file path to find, default current directory | not |
| value      | string | finded content | yes |

The optional parameter filterObj supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Find a file;

```lang-javascript
> File.find( { mode: 'n', value: "file.txt", pathname: "/opt" } )
{
    "pathname": "/opt/sequoiadb1/file.txt"
}
{
    "pathname": "/opt/sequoiadb2/file.txt"
}
{
    "pathname": "/opt/sequoiadb3/file.txt"
}
```

* Find a file and filter the result set

```lang-javascript
> File.find( { mode: 'n', value: "file.txt", pathname: "/opt" }, { $or: [ { pathname: "/opt/sequoiadb1/file.txt" }, { pathname: "/opt/sequoiadb2/file.txt" } ] } )
{
  "pathname": "/opt/sequoiadb1/file.txt"
}
{
  "pathname": "/opt/sequoiadb2/file.txt"
}
```