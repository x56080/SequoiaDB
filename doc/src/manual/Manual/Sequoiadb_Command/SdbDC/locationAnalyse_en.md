##NAME##

locationAnalyse - analyze the Location distribution rationality in the cluster

##SYNOPSIS##

**SdbDC.locationAnalyse([option], [fileName])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to analyze whether the Location distribution in the cluster is reasonable. By querying SDB_LIST_GROUPS and SDB_LIST_GROUPMODES, it parses the Location settings, ActiveLocation status, and GroupMode (Critical/Maintenance) status of each node, and outputs detailed analysis information and exception information for each Location.

##PARAMETERS##

option ( *object, optional* )

Specify filter conditions through the parameter "option". When set to null or not specified, all groups (including coord group and data groups) will be analyzed:

- HostName ( *string* ): Filter by hostname, only analyze groups that contain nodes on the specified host

    Format: `HostName: "sdbserver"`

- Domain ( *string | string array* ): Filter by domain, only analyze groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

fileName ( *string, optional* )

Specify the output file path. When specified, the analysis result will be written to the file in JSON format (2-space indentation).

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedHostNum | number | Number of matched hosts |
| MatchedGroupNum | number | Number of matched groups (including coord group) |
| MatchedNodeNum | number | Number of matched nodes |
| ActiveLocation | string / array | ActiveLocation of all data groups. Returns a string if all data groups have the same ActiveLocation; returns a sorted array if different; returns an empty string if not set |
| LocationInfo | array | List of Location detail information |
| ExceptionHostInfo | object | Host exception information |
| ExceptionGroupInfo | object | Group exception information |

Each element in the LocationInfo array contains the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| LocationName | string | Location name |
| ActiveStatus | string | ActiveLocation status. Values: "All" (all data groups have this Location as ActiveLocation), "None" (no data group has this Location as ActiveLocation), "Partical" (some data groups have this Location as ActiveLocation) |
| GroupStatus | string | GroupMode status, see the table below for values |
| WholeHost | array | List of hostnames where this Location covers all matched nodes on the host |
| ParticalHost | array | List of host information where this Location covers only some matched nodes on the host. Each element contains HostName and Node fields |

GroupStatus values:

| Value | Description |
| ----- | ----------- |
| "" | No GroupMode is set for any data group that this Location belongs to |
| "Critical" | All data groups that this Location belongs to have Critical mode set for this Location |
| "ParticalCritical" | Some data groups that this Location belongs to have Critical mode set for this Location |
| "Maintenance" | All data groups that this Location belongs to have Maintenance mode set for nodes in this Location |
| "ParticalMaintenance" | Some data groups that this Location belongs to have Maintenance mode set for nodes in this Location |
| "Critical-Maintenance" | All data groups that this Location belongs to have either Critical or Maintenance mode set |
| "Partical-Critical-Maintenance" | Both Critical and Maintenance modes exist among data groups, but some data groups have no mode set |

ExceptionHostInfo contains the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| NoLocationHost | array | List of hostnames where all nodes have no Location set |
| ParticalLocationHost | array | List of hostnames where some nodes have Location set and some do not |
| MultyLocationHost | array | List of hostnames where nodes are distributed across multiple different Locations |

ExceptionGroupInfo contains the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| NoLocationGroup | array | List of group names where all nodes have no Location set |
| ParticalLocationGroup | array | List of group names where some nodes have Location set and some do not |
| OneLocationGroup | array | List of group names where all nodes with Location set are in the same Location |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `locationAnalyse()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error | Check whether the parameter type is correct |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Analyze Location distribution of all groups

```lang-javascript
> var dc = db.getDC()
> dc.locationAnalyse()
{
  "MatchedHostNum": 3,
  "MatchedGroupNum": 4,
  "MatchedNodeNum": 12,
  "ActiveLocation": "GuangZhou",
  "LocationInfo": [
    {
      "LocationName": "GuangZhou",
      "ActiveStatus": "All",
      "GroupStatus": "",
      "WholeHost": [
        "sdbserver1"
      ],
      "ParticalHost": [
        {
          "HostName": "sdbserver2",
          "Node": [
            "sdbserver2:20000",
            "sdbserver2:20010"
          ]
        }
      ]
    },
    {
      "LocationName": "ShenZhen",
      "ActiveStatus": "None",
      "GroupStatus": "Critical",
      "WholeHost": [],
      "ParticalHost": [
        {
          "HostName": "sdbserver2",
          "Node": [
            "sdbserver2:20020"
          ]
        },
        {
          "HostName": "sdbserver3",
          "Node": [
            "sdbserver3:20000",
            "sdbserver3:20010"
          ]
        }
      ]
    }
  ],
  "ExceptionHostInfo": {
    "NoLocationHost": [],
    "ParticalLocationHost": [
      "sdbserver3"
    ],
    "MultyLocationHost": [
      "sdbserver2"
    ]
  },
  "ExceptionGroupInfo": {
    "NoLocationGroup": [],
    "ParticalLocationGroup": [
      "db3"
    ],
    "OneLocationGroup": [
      "db1"
    ]
  }
}
```

**Example 2:** Filter by domain and save results to a file

```lang-javascript
> var dc = db.getDC()
> dc.locationAnalyse({Domain: "mydomain"}, "/tmp/location_report.json")
{
  "MatchedHostNum": 2,
  "MatchedGroupNum": 2,
  "MatchedNodeNum": 6,
  "ActiveLocation": "GuangZhou",
  "LocationInfo": [
    {
      "LocationName": "GuangZhou",
      "ActiveStatus": "All",
      "GroupStatus": "",
      "WholeHost": [
        "sdbserver1"
      ],
      "ParticalHost": []
    }
  ],
  "ExceptionHostInfo": {
    "NoLocationHost": [],
    "ParticalLocationHost": [],
    "MultyLocationHost": []
  },
  "ExceptionGroupInfo": {
    "NoLocationGroup": [],
    "ParticalLocationGroup": [],
    "OneLocationGroup": [
      "db1"
    ]
  }
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
