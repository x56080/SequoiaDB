##名称##

getUser - 获取用户的信息

##语法##

**db.getUser( <username>, [options] )**

##类别##

Sdb

##描述##

该函数用于获取指定用户的信息

##参数##

* username （ _string，必填_ ） 指定用户

* options （_object，选填_） 额外参数
  * ShowPrivileges （ _boolean_ ） 展示用户的权限，默认值为 `false`

##返回值##

函数执行成功时，打印角色的信息。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v7.0 及以上版本

##示例##

- 在集群中获取名为 `myuser` 的角色，不开启 `ShowPrivileges` 选项

  ```lang-javascript
  > db.getUser("myuser")
  {
    "User": "myuser",
    "Roles": [
        "_foo.read"
    ],
    "Options": {}
  }
  ```

- 在集群中获取名为 `myuser` 的角色，开启 `ShowPrivileges` 选项

  ```lang-javascript
  > db.getUser("myuser",{ShowPrivileges:true})
    {
        "User": "myuser",
        "Roles": [
            "_foo.read"
        ],
        "Options": {},
        "InheritedRoles": [
            "_foo.read"
        ],
        "InheritedPrivileges": [
            {
            "Resource": {
                "cs": "foo",
                "cl": ""
            },
            "Actions": [
                "find",
                "getDetail"
            ]
            }
        ]
    }
  ```


[^_^]: 
    本文使用的所有引用及链接
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
