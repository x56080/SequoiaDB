## 名称

removeGroups - 从域中删除复制组

## 语法

**domain.removeGroups(\<object\>)**


## 类别

SdbDomain

## 描述

该函数用于删除域中复制组。

   >**Note:**
   >
   >删除复制组前必须保证组内不包含任何数据，否则操作将失败。 

## 参数

options ( *object，必填* )

需要修改的属性列表

-  Groups（string/array）：域将删除的复制组

   格式：`Groups:['group1','group2']`

## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

| 错误码 | 可能的原因   | 解决方法              |
| ------ | ------------ | --------------------- |
| -154   | 分区组不存在 | 使用列表查看分区组是否存在 |
| -256   | 域已被使用   | 使用 domain.listCollectionSpaces() 查看域是否存在集合空间 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error]。更多错误处理可以参考[常见错误处理指南][faq]。

## 版本

v2.0 及以上版本

## 示例

创建一个包含三个复制组的域，并开启自动切分

```lang-javascript
> var domain = db.createDomain('mydomain',['group1','group2','group3'],{AutoSplit:true})
```

* 从域中删除复制组 group2

   ```lang-javascript
   > domain.removeGroups( { Groups: ['group2'] } )
   ```

* 从域中删除复制组 group1 和 group2

    ```lang-javascript
   > domain.removeGroups( { Groups: ['group1','group2'] } )
   ```  


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[error]:manual/Manual/Sequoiadb_error_code.md