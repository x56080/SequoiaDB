##NAME##

list - Enumerate domains

##SYNOPSIS##

**db.list( \<listType\>, [cond], [sel], [sort] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate all lists.The list is a lightweight command to get the current system status.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| listType | Enumerate | [List type][List_type]| Required 	   |
| cond     | Json      | Match condictions and [position parameter][parameter]. | Not 	   |
| sel      | Json      | Select the returned field name. When it is null, return all field names.         | Not 	   |
| sort     | Json      | Sort the returned records by the selected field. 1 is ascending and -1 is descending.        | Not 	   |

>**Note:**

>* For the value of the listType,refer to [ListType][list_info].
>* sel parameter is a json structure,like:{Field name:Field value}，The field value is generally specified as an empty string.The field name specified in sel exists in the record,setting the field value does not take effect;return the field name and field value specified in the sel otherwise.
>* The field value type in the record is an array.User can specify the field name in sel,and use "." operator with double marks ("") to refer to the array elements.

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* Specify the value of listType as SDB_LIST_CONTEXTS

	```lang-javascript
	> db.list( SDB_LIST_CONTEXTS )
	{
	  "NodeName": "ubuntu-200-043:11850",
	  "SessionID": 29,
	  "TotalCount": 1,
	  "Contexts": [
		254
	  ]
	}
	```

* Specify the value of listType as SDB_LIST_STORAGEUNITS

	```lang-javascript
	> db.list( SDB_LIST_STORAGEUNITS )
    {
      "NodeName": "ubuntu-200-043:11830",
      "Name": "sample",
      "UniqueID": 61,
      "ID": 4094,
      "LogicalID": 186,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "Sequence": 1,
      "NumCollections": 1,
      "CollectionHWM": 1,
      "Size": 306315264
    }
	```

* Return reccords with LogicalID greater than 1 and only return the Name field and ID field for each record.The records are in ascending order according to the value of the Name field

	```lang-javascript
	> db.list( SDB_LIST_STORAGEUNITS, { "LogicalID": { $gt: 1 } }, { Name: "", ID: "" }, { Name: 1 } )
	{
	  "Name": "sample",
	  "ID": 4094
	}
	```

* Specify the command position parameter and only return the context of the data group db1

	```lang-javascript
	> db.list( SDB_LIST_CONTEXTS, { GroupName: "db1" } )
	{
	  "NodeName": "ubuntu-200-043:20000",
	  "SessionID": 29,
	  "TotalCount": 1,
	  "Contexts": [
		254
	  ]
	}
	```


[^_^]:
     links
[List_type]:manual/Manual/List/list.md
[parameter]:manual/Manual/Sequoiadb_Command/location.md
[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md