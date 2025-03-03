
##NAME##

lobExplain - Obtain the execution plan for retrieving the shard location information of large objects in the collection

##SYNOPSIS##
***db.collectionspace.collection.lobExplain(\<Oid\>, [detail], [options])***

##CATEGORY##

Collection

##DESCRIPTION##

Obtain the execution plan for retrieving the shard location information of large objects in the collection.

##PARAMETERS##

* `Oid`( *String*， *Required* )
  
    Lob's ID

* detail ( *boolean, optional* )

    Whether to display detailed pieces in the location information, default is false if omitted.

* options ( *object, optional* )

    The extended control options are as follows:

    - Offset: The starting position for large object computation.
    - Length: The length for large object computation.


  > **Note:**
  >
  > - When the large object exists, `Offset` defaults to 0, and `Length` defaults to the size of the large object. If the large object size exceeds 2GB, it will be computed based on 2GB. The values can be controlled by specifying `Offset` and `Length`.
  > - When the large object does not exist, `Offset` defaults to 0, and `Length` defaults to 512KB.
  > - If the specified `Length` exceeds 2GB (2147483648), it will be adjusted to 2GB.


##RETURN VALUE##

On success, return the lob's runtime detail information.

On error, exception will be thrown.

##ERRORS##

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##EXAMPLES##

1. Obtain the execution plan for retrieving the LOB shards where Oid is "000067bce21e330004538d02".

	```lang-javascript
    > db.sample.lob.lobExplain('000067bce21e330004538d02')
	{
      "Oid": "000067bce21e330004538d02",
      "LobPageSize": 262144,
      "Exist": true,
      "Offset": 0,
      "Length": 2597114,
      "GroupID": 1001,
      "Location": [
        {
          "GroupID": 1000,
          "PiecesNum": 3
        },
        {
          "GroupID": 1001,
          "PiecesNum": 6
        },
        {
          "GroupID": 1006,
          "PiecesNum": 1
        }
      ],
      "PiecesNum": 10
	}
	```