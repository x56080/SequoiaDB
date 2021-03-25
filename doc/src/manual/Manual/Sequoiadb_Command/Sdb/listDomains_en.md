##NAME##

listDomains - Enumerate domains

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

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [SYSDOMAINS colletion][SYSDOMAINS] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

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

[^_^]:
     links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSDOMAINS]:manual/Manual/Catalog_Table/SYSDOMAINS.md