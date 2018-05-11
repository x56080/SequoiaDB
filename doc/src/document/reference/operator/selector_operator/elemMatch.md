##语法##

```
{ <字段名1>: { $elemMatch: <表达式1> }, <字段名2>: { $elemMatch: <表达式2> }, ... }
```

##描述##

返回数组内满足条件的元素的集合，或者嵌套对象中满足条件的子对象。

其中“<表达式>”可以是值，也可以是带有[匹配符](reference/operator/match_operator/overview.md)的表达式，“$elemMatch”匹配符支持多层嵌套。

##示例##

在集合 foo.bar 插入 3 条记录，两条是数组类型，一条是嵌套对象类型

```lang-javascript
> db.foo.bar.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] } )
> db.foo.bar.insert( { "_id": 2, "class": 2, "students": { "name": "LinWu", "age": 18 } } )
> db.foo.bar.insert( { "_id": 3, "class": 3, "students": [ { "name": "ZhangSan", "age": 18, course: [ { math: 1 }, { english: 0 } ] }, { "name": "LiSi", "age": 19, course: [ { math: 1 }, { english: 1 } ] }, { "name": "WangErmazi", "age": 18, course: [ { math: 0 }, { english: 0 } ] } ] } )
> db.foo.bar.find()
{
  "_id": 1,
  "class": 1,
  "students": [
    {
      "name": "ZhangSan",
      "age": 18
    },
    {
      "name": "LiSi",
      "age": 19
    },
    {
      "name": "WangErmazi",
      "age": 18
    }
  ]
}
{
  "_id": 2,
  "class": 2,
  "students": {
    "name": "LinWu",
    "age": 18
  }
}
{
  "_id": 3,
  "class": 3,
  "students": [
    {
      "name": "ZhangSan",
      "age": 18,
      "course": [
        {
          "math": 1
        },
        {
          "english": 0
        }
      ]
    },
    {
      "name": "LiSi",
      "age": 19,
      "course": [
        {
          "math": 1
        },
        {
          "english": 1
        }
      ]
    },
    {
      "name": "WangErmazi",
      "age": 18,
      "course": [
        {
          "math": 0
        },
        {
          "english": 0
        }
      ]
    }
  ]
}
Return 3 row(s).
```

SequoiaDB shell 运行如下：

* 指定返回“age”等于 18 的元素：

  ```lang-javascript
  > db.foo.bar.find( {}, { "students": { "$elemMatch": { "age": 18 } } } )
    {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18
        },
        {
          "name": "WangErmazi",
          "age": 18
        }
      ]
    }
    {
      "_id": 2,
      "class": 2,
      "students": {
        "name": "LinWu",
        "age": 18
      }
    }
    {
      "_id": 3,
      "class": 3,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18,
          "course": [
            {
              "math": 1
            },
            {
              "english": 0
            }
          ]
        },
        {
          "name": "WangErmazi",
          "age": 18,
          "course": [
            {
              "math": 0
            },
            {
              "english": 0
            }
          ]
        }
      ]
    }
    Return 3 row(s).
  ```

* 指定返回“age”大于 18 的元素，使用“$gt”表达式：

  ```lang-javascript
  > db.foo.bar.find( { "class": 1 }, { "students": { "$elemMatch": { "age": { $gt: 18 } } } } )
  {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "LiSi",
          "age": 19
        }
      ]
  }
  Return 1 row(s).
  ```
* 指定返回姓“Wang”的元素，使用“$regex”表达式：

  ```lang-javascript
  > db.foo.bar.find( { "class": 1 }, { "students": { "$elemMatch": { "name": { $regex: "^Wang.*" } } } } )
  {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "WangErmazi",
          "age": 18
        }
      ]
  }
  Return 1 row(s).
  ```

* 指定返回 3 班学生数组中选择了数学的元素，使用嵌套的“$elemMatch”表达式：

  ```lang-javascript
  > db.foo.bar.find( { class: 3 }, { students: { $elemMatch: { course: { $elemMatch: { math: 1 } } } } } )
{
  "_id": 3,
  "class": 3,
  "students": [
    {
      "name": "ZhangSan",
      "age": 18,
      "course": [
        {
          "math": 1
        },
        {
          "english": 0
        }
      ]
    },
    {
      "name": "LiSi",
      "age": 19,
      "course": [
        {
          "math": 1
        },
        {
          "english": 1
        }
      ]
    }
  ]
}
  Return 1 row(s).
  ```
