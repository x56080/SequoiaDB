##名称##

setAttributes - 修改域的属性

##语法##

**domain.setAttributes(\<options\>)**

##类别##

SdbDomain

##描述##

该函数用于修改域的属性。

##参数##

options ( *object，必填* )

通过参数 options 可以修改域的属性：

-  Groups ( *string/array* )：域包含的复制组

    修改域包含的复制组时，如果涉及删除复制组操作，需保证该复制组中不存在数据，否则操作报错。

    格式：`Groups: ['group1', 'group2']`

-  AutoSplit ( *boolean* )：是否开启自动切分，默认值为 false，不开启

    该参数修改后，仅对新创建的集合空间和集合生效。

    格式：`AutoSplit: true`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setAttributes()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -154   | SDB_CLS_GRP_NOT_EXIST |分区组不存在 | 使用列表查看分区组是否存在 |
| -214   | SDB_CAT_DOMAIN_NOT_EXIST| 域不存在     | 使用 [listDomains()](reference/Sequoiadb_command/Sdb/listDomains.md) 查看域是否存在 |
| -256   | SDB_DOMAIN_IS_OCCUPIED |域已被使用   | 使用 [listCollectionSpaces()](reference/Sequoiadb_command/SdbDomain/listCollectionSpaces.md) 查看域是否存在集合空间 |

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

- 创建一个包含复制组 group1 和 group2 的域，复制组中不存在数据

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'])
    ```
  
    将域包含的复制组修改为 group1 和 group3

    ```lang-javascript
    > domain.setAttributes({Groups: ['group1', 'group3']})
    ```

- 创建一个包含复制组 group1 的域，复制组中存在数据

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

    将域包含的复制组修改为 group2
 
    ```lang-javascript
    > domain.setAttributes({Groups: ['group2']})
    ```

    由于 group1 中存在数据，操作报错
   
    ```lang-javascript
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```