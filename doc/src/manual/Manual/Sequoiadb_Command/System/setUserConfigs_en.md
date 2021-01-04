
##NAME##

setUserConfigs - Setting user configurations

##SYNOPSIS##

***System.setUserConfigs( \<options\> )***

##CATEGORY##

System

##DESCRIPTION##

Setting user configurations

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| options  | JSON   | ---    |  new user configuration   | yes       |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | ---------------- |
| name    | String |    yes   | { name: "username" }     |  username     |
| Group    | String |  not   | { Group: "groupname" }     |  new group name     |
| dir    | String |  not   | { dir: "dir" }     |  new home directory    |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Modiify the home directory of users in the specified user group 

```lang-javascript
> System.setUserConfigs( { "name": "username", "Group": "groupname", "dir": "/home/username" } )
```