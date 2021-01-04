
##语法##

```lang-json
{ <字段名.$+标识符>: <value> }
```


##描述##

$+标识符是一种特殊的命令符，这种命令符只作用于数组对象，用来代替数组的索引 Key，并且可以把匹配到的第一个索引值传递到方法 [update][up] 的 rule 参数中。

标识符是一个整数，如 $1，$3，标识符相当于一个临时的存储，会把匹配成功的数组元素的索引存储起来。错误的书写格式如：$5.4，$a2，$3c，$MA。


##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "a": [ 1, 2, 3, 4, 5 ] } )
> db.sample.employee.insert( { "a": [ 1, 4, 5 ] } )
> db.sample.employee.insert( { "a": [ 4, 2, 1 ] } )
```

* 查询出字段 a 的数组中存在元素 5 的记录

  ```lang-javascript
  > db.sample.employee.find( { "a.$1": 5 }, { "a": 1 } )
  {
      "a": [
        1,
        2,
        3,
        4,
        5
      ]
  }
  {
      "a": [
        1,
        4,
        5
      ]
  }
  Return 2 row(s).
  ```

* 修改数组 a 中的元素，把值为 4 的元素改成 100

  ```lang-javascript
  > db.sample.employee.update( { "$set": { "a.$1": 100 } }, { "a.$1": 4 } )
  > db.sample.employee.find()
  {
      "_id": {
        "$oid": "582450162b4c38286d000014"
      },
      "a": [
        1,
        2,
        3,
        100,
        5
      ]
  }
  {
      "_id": {
        "$oid": "5824501b2b4c38286d000015"
      },
      "a": [
        1,
        100,
        5
      ]
  }
  {
      "_id": {
        "$oid": "582450202b4c38286d000016"
      },
      "a": [
        100,
        2,
        1
      ]
  }
  Return 3 row(s).
  ```

*  修改数组 a 中的元素，把值为 100 的元素改成 2，且把 1 修改为 200

  ```lang-javascript
  > db.sample.employee.update( { "$set": { "a.$1": 2, "a.$2": 200 } }, { "a.$1": 100, "a.$2": 1 } )
  > db.sample.employee.find()
  {
      "_id": {
        "$oid": "582450162b4c38286d000014"
      },
      "a": [
        200,
        2,
        3,
        2,
        5
      ]
  }
  {
      "_id": {
        "$oid": "5824501b2b4c38286d000015"
      },
      "a": [
        200,
        2,
        5
      ]
  }
  {
      "_id": {
        "$oid": "582450202b4c38286d000016"
      },
      "a": [
        2,
        2,
        200
      ]
  }
  Return 3 row(s).
  ```

* 修改数组 a 中的元素，把值为 2 的元素改成 2000

  ```lang-javascript
  > db.sample.employee.update( { "$set" : { "a.$1" : 2000 } }, { "a.$1": 2 } )
  > db.sample.employee.find()
  {
      "_id": {
        "$oid": "582450162b4c38286d000014"
      },
      "a": [
        200,
        2000,
        3,
        2,
        5
      ]
  }
  {
      "_id": {
        "$oid": "5824501b2b4c38286d000015"
      },
      "a": [
        200,
        2000,
        5
      ]
  }
  {
      "_id": {
        "$oid": "582450202b4c38286d000016"
      },
      "a": [
        2000,
        2,
        200
      ]
  }
  Return 3 row(s).
  ```

  > **Note:**  
  >
  > 如果有多个元素符合规则，那么只会修改第一个。



[^_^]:
     本文使用的所有引用及链接
[up]:manual/Manual/Sequoiadb_Command/SdbCollection/update.md