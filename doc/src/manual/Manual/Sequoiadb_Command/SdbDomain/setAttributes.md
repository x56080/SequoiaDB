##名称##

setAttributes - 修改域的属性

##语法##

**domain.setAttributes(\<options\>)**

##类别##

SdbDomain

##描述##

该函数用于修改域的属性。

>**Note:**
>
> - 删除复制组前必须保证组内不包含任何数据，否则操作将失败。
> - 更改 AutoSplit 不会对之前创建的集合和集合空间产生影响。

##参数##

options ( *object，必填* )

通过 options 参数可以修改域的属性：

-  Groups ( *string/array* )：域将包含的所有复制组

   格式：`Groups: ['group1', 'group2']`

-  AutoSplit ( *boolean* )：域是否自动切分

   格式：`AutoSplit: true|false`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setAttributes()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -154   | SDB_CLS_GRP_NOT_EXIST |分区组不存在 | 使用列表查看分区组是否存在 |
| -214   | SDB_BAR_BACKUP_NOTEXIST |域不存在 | 使用 listDomains() 查看域是否存在 |
| -256   | SDB_DOMAIN_IS_OCCUPIED |域已被使用   | 使用 listCollectionSpaces() 查看域是否存在集合空间 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 首先创建一个域，包含两个复制组，开启自动切分

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
    ```

- 从域中删除一个复制组 group2，添加另一个复制组 group3，最后域中包含 group1 和 group3 两个复制组

    ```lang-javascript
    > domain.setAttributes({Groups: ['group1', 'group3']})
    ```

- 首先创建一个域，包含一个复制组，复制组中包含表 sample.employee

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

- 从域中删除原复制组，添加另一个复制组，将因把拥有数据的 group1 从域中删除而报错

    ```lang-javascript
    > domain.setAttributes({Groups: ['group2']})
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md