更新操作即修改集合中已存在的记录。SequoiaDB中使用 [update()](reference/Sequoiadb_command/SdbCollection/update.md)方法做更新操作。

>**Note:** 本文档的所有例子都是使用 SequoiaDB 的 shell 接口。

##update()##

update() 方法是修改集合中记录的主要方法

##使用 update 操作修改记录##

如果 update() 方法只有 rule 参数时（例如使用 $set 更新表达式），那么 update 方法会修改集合记录中所有指定的字段；更新嵌套对象 SequoiaDB 使用点操作符（.）。

-   更新记录字段

    使用 [$set](reference/operator/update_operator/set.md) 更新记录字段的值。下面的操作修改集合 bar 中符合条件 \_id 字段值等于1的记录，使用 $set 修改 name 字段的嵌套元素 first字段的值，将它的值修改为“Mike”：

    ```lang-javascript
    > db.foo.bar.update( { $set: { "name.first": "Mike" } }, { _id: 1 } )
    ```

    > **Note:**
    > 
    > 如果 rule 参数包含的字段名没有在当前的记录中，update()方法会添加 rule 参数包含的字段到记录中。

-   删除记录字段

    使用 [$unset](reference/operator/update_operator/unset.md) 删除记录的字段名。下面的操作是删除集合 bar 中所有记录中的 age 字段，如果记录中没有 age 字段，则跳过。

    ```lang-javascript
    > db.foo.bar.update( { $unset: { age: "" } } )
    ```

-   数组元素更新

    如果需要更新数组中的元素，SequoiaDB使用点操作符（.），数组下标从0开始。下面的操作是修改数组字段 arr 的第二个元素的值，将它的值增加5：

    ```lang-javascript
    > db.foo.bar.update( { $inc: { "arr.1": 5 } } )
    ```