
##NAME##

getLobDetail - Get lob's runtime detail information

##SYNOPSIS##
***db.collectionspace.collection.getLobDetail(\<Oid\>, [detail])***

##CATEGORY##

Collection

##DESCRIPTION##

Get lob's runtime detail information

##PARAMETERS##

* `Oid`( *String*， *Required* )
  
    Lob's ID

* detail ( *boolean, optional* )

    Whether to display detailed pieces in the location information, default is false if omitted.


  > **Note:**
  >
  > When the LOB size exceeds 2GB, the `Location` and `PiecesNum` will be calculated based on 2GB.

##RETURN VALUE##

On success, return the lob's runtime detail information.

On error, exception will be thrown.

##ERRORS##

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##EXAMPLES##

1. Get the lob's runtime detail information which lob's ID is 00005deb85c5350004743b09

	```lang-javascript
    > db.sample.employee.getLobDetail('00005deb85c5350004743b09')
	{
	  "Oid": "00005deb85c5350004743b09",
	  "AccessInfo": {
	    "RefCount": 3,
	    "ReadCount": 0,
	    "WriteCount": 1,
	    "ShareReadCount": 2,
	    "LockSections": [
	      {
	        "Begin": 10,
	        "End": 30,
	        "LockType": "X",
	        "Contexts": [
	          11
	        ]
	      },
	      {
	        "Begin": 30,
	        "End": 50,
	        "LockType": "S",
	        "Contexts": [
	          12
	        ]
	      }
	    ]
	  },
      "GroupID": 1001,
	  "ContextID": 14,
      "Location": [
        {
          "GroupID": 1000,
          "PiecesNum": 107
        },
        {
          "GroupID": 1001,
          "PiecesNum": 120
        },
        {
          "GroupID": 1006,
          "PiecesNum": 123
        }
      ],
      "PiecesNum": 350,
      "Size": 91635840,
      "CreateTime": {
        "$timestamp": "2025-02-21-16.33.44.460000"
      },
      "ModificationTime": {
        "$timestamp": "2025-02-21-16.33.46.975000"
      },
      "Version": 2,
      "Available": true,
      "Flag": 0,
      "HasPiecesInfo": false,
      "PiecesInfoNum": 0
	}
	```