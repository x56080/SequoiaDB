##NAME##

getLob - read LOB

##SYNOPSIS##

**db.collectionspace.collection.getLob\(\<oid\>, \<file path\>, \[forced\]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to read Lob in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| --------- | -------- | ------ | -------- |
| oid       | string   | Unique descriptor for Lob             | required |
| file path | string   | The full path of the local file to be written          | required |
| forced    | boolean     |Overwrite local file if they already exists.| not |

> **Note:**
>
> - Local files do not need to be created manually in advance.
> - The "forced" is false by default.

##RETURN VALUE##

When the function executes successfully, it will return an object of type String.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Write the LOB with the identifier "5435e7b69487faa663000897" to the local `/opt/newlob` file.

```lang-javascript
> db.sample.employee.getLob('5435e7b69487faa663000897', '/opt/newlob')
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
