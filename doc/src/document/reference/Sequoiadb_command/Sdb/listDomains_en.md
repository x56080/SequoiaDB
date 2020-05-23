##NAME##

listDomains - Enumerate domains.

##SYNOPSIS##

**db.listDomain( [cond], [sel], [sort] )**

##CATEGORY##

Sdb

##DESCRIPTION##

Enumerate all user-created domains in the system. 

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| cond    | Json   | Match condictions, and only return records that match cond. When null, return all.              | Not             |
| sel     | Json   | Select the returned field name. When null, return all field names.                        | Not             |
| sort    | Json   | Sort the returned records by the selected field. 1 is ascending and -1 is descending.        | Not             |

##RETURN VALUE##

On success, return the cursor object.

On error, exception will be thrown.  Users can get the error message by [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) or get the error code by [getLastError()](reference/Sequoiadb_command/Global/getLastError.md).  For error handling, refer to the common [troubleshooting](troubleshooting/general/general_guide.md).

##ERRORS##

For common errors, refer to [error code](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Enumerate all user-created domains in the system.

```lang-javascript
> db.listDomains()
{
  "_id": {
	"$oid": "5811641e3426f0835eef45bf"
  },
  "Name": "mydomain",
  "Groups": [
	{
	  "GroupName": "group1",
	  "GroupID": 1001
	},
	{
	  "GroupName": "group2",
	  "GroupID": 1002
	},
	{
	  "GroupName": "group3",
	  "GroupID": 1000
	}
  ]
}
```
