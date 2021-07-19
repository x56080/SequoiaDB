##NAME##

putLob - insert a LOB in the collection

##SYNOPSIS##

**db.collectionspace.collection.putLob\(\<file path\>, [oid]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to insert LOB in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| --------- | -------- | -------- | -------- |
| file path | string   | The absolute path of the file to be uploaded, and users need to have the read permission of the file. | required |
| oid       | string   | Specify the oid of the LOB. | not |

##RETURN VALUE##

When the function executes successfully, it will return an oid string of type String. Users can perform related operations on LOB through oid.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Create collection space and collection.

    ```lang-javascript
    > db.createCS('sample' )
    > db.sample.createCL('employee')
    ```

- Upload the LOB file `mylob.txt`.

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt')
    0000604f989a390002db009e
    ```

- Upload the LOB file `mylob.txt` of the specified "oid".

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt', '5bf3a024ed9954d596420256')
    5bf3a024ed9954d596420256
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
