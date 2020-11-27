我们使用 [find()](reference/Sequoiadb_command/SdbCollection/find.md) 方法读取 SequoiaDB 中的记录。find方法是从集合中选择记录的主要方法，它返回一个包含很多记录的游标

>**Note:** 本文档的所有例子都是使用 SequoiaDB 的 shell 接口。

##find()##

find() 方法是查询集合中记录的主要方法

##返回集合所有记录##

如果没有任何参数，则返回集合中所有的记录。以下示例为返回集合空间 foo 中集合 bar 的所有记录：

```lang-javascript
> db.foo.bar.find()
```


现集合中有如下一条记录：

```lang-json
{
  "_id": 1,
  "name":
    {
      "first": "Tom",
      "second": "David"
    },
  "age": 23
  "birth": "1990-04-01",
  "phone":
    [
      10086,
      10010,
      10000
    ],
  "family":
    [
      {
        "Dad": "Kobe",
        "phone": 139123456
      },
      {
        "Mom": "Julie",
        "phone": 189123456
      }
    ]
 }
```

##返回匹配条件的记录##

-   Equality 匹配

    下面的操作返回集合 bar 中 age 等于23的记录

    ```lang-javascript
    > db.foo.bar.find( { age: 23 } )
    ```

-   使用[匹配符](reference/operator/match_operator/overview.md)

    下面操作返回集合 bar 中 age 字段值大于20的记录

    ```lang-javascript
    > db.foo.bar.find( { age: { $gt: 20 } } )
    ```

-   嵌套数组匹配

    1.数组元素查询，下面的操作返回集合 bar 中所有字段 phone （phone为数组类型）含有元素10086的记录：

    ```lang-javascript
    > db.foo.bar.find( { "phone": 10086 } )
    ```

    2.数组元素为 BSON 对象的查询，下面的操作返回集合 bar 中 family 字段包含的子元素 Dad 字段值为“Kobe”，phone字段值为139123456的记录：

    ```lang-javascript
    > db.foo.bar.find( { "family": { $elemMatch: { "Dad": "Kobe", "phone": 139123456 } } } )
    ```

-   嵌套 BSON 对象匹配查询

    下面的操作返回一个游标指向集合 bar 中嵌套 BSON 对象的 name 字段匹配{ "first": "Tom" }的记录

    ```lang-javascript
    > db.foo.bar.find( { "name": { "$elemMatch": { "first": "Tom" } } } )
    ```

    上面还可以写成：

    ```lang-javascript
    > db.foo.bar.find( { "name.first": "Tom" } )
    ```

##指定返回记录字段##

如果指定 find 方法的 sel 参数，那么只返回指定的 sel 参数内的字段名。下面的操作返回记录的 name 字段：

```lang-javascript
> db.foo.bar.find( {}, { name: "" } )
```

> **Note:**
> 
> 如果记录中不存在指定的字段名（如：people），SequoiaDB 默认也返回。如：
> 
> ```lang-javascript
> > db.foo.bar.find( {}, { name: "", people: "" } )
> {
>   "name":
>   {
>     "fist": "Tom",
>     "second": "David"
>   },
>   "people": ""
> }
> ```

##更多信息##

执行 db.foo.bar.find().help() , 会看到 find() 的更多使用方法

-   cursor.sort( &lt;sort&gt; )

    sort()方法用来按指定的字段排序，语法格式为：sort( { 字段名1: 1|-1, 字段名2: 1|-1, ...} )，1为升序，-1为降序。如：

    ```lang-javascript
    > db.foo.bar.insert( { "name": "Tom", "age": 20 } )
    > db.foo.bar.insert( { "name": "Anna", "age": 22 } )
    > db.foo.bar.find().sort( { age: -1 } )
    {
      "name": "Anna",
      "age": 22
    }
    {
      "name": "Tom",
      "age": 20
    }
    ```

-   cursor.hint( &lt;hint&gt; )

    添加[索引](manual/Distributed_Engine/Architecture/Data_Model/index.md)加快查找速度，假设存在名为“testIndex”的索引：

    ```lang-javascript
    > db.foo.bar.find().hint( { "": "testIndex" } )
    ```

-   cursor.limit( &lt;num&gt; )

    在结果集中限制返回的记录条数：

    ```lang-javascript
    > db.foo.bar.find().limit( 3 )
    ```

    返回结果集里面的的前三条记录

-   cursor.skip( &lt;num&gt; )

    skip() 方法控制结果集的开始点，即跳过前面的 num 条记录，从num+1条记录开始返回：

    ```lang-javascript
    > db.foo.bar.find().skip( 5 )
    ```

    从查询的结果集的第6条记录开始返回

-   使用游标控制 find() 返回的记录

    ```lang-javascript
    db.foo.bar.find().current()   //返回当前游标指向的记录
    db.foo.bar.find().next()      //返回当前游标指向的下一条记录
    db.foo.bar.find().close()     //关闭当前游标，当前游标不再可用
    db.foo.bar.find().count()     //返回当前游标的记录总数
    db.foo.bar.find().size()      //返回当前游标到最终游标的距离
    db.foo.bar.find().toArray()   //以数组形式返回结果集
    ```
