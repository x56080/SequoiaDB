##语法##

```
{ <字段名>: { $slice: <值> } }
```

##说明##

截取数组中的子数组。字段非数组时，返回原值。

##格式##

从第一个元素开始选取，其中 value 指要取的元素个数：

```
find({},{<fieldName>:{<$slice:Value>}})
```

或者，从某个元素开始选取数个元素，其中 value1 指从第几个元素开始取值（value1 取 0 表示为第一个元素），value2 指要取的元素个数：

```
find({},{<fieldName>:{<$slice:[value1,value2]>}})
```

value1 可以为负数，指从倒数第几个开始取值。不管value1取正数还是负数，取值都是正序取值。
指定要取的元素个数不足时,只取开始取值的元素到最后一个元素的集合。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript
> db.foo.bar.insert( { "a": [ 1, 2, 3, 4, 5 ] } )
```

SequoiaDB shell 运行如下：

1. 作为选择符使用：

  从第1个元素开始取值，取2个元素：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$slice": 2 } } )
  {
      "_id": {
        "$oid": "58257889ec5c9b3b7e000000"
      },
      "a": [
        1,
        2
      ]
  }
  Return 1 row(s).
  ```

  从倒数第2个元素开始取值，取2个元素：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$slice": -2 } } )
  {
      "_id": {
        "$oid": "58257889ec5c9b3b7e000000"
      },
      "a": [
        4,
        5
      ]
  }
  Return 1 row(s).
  ```

  从第3个元素开始取值，取3个元素：
  
  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$slice": [ 2, 3 ] } } )
  {
      "_id": {
        "$oid": "58257889ec5c9b3b7e000000"
      },
      "a": [
        3,
        4,
        5
      ]
  }
  Return 1 row(s).
  ```

  从倒数第2个元素开始取值，取3个元素 （长度不足时，从倒数第2个元素开始取值，取完后面所有的元素，实际取了2个元素）：
  
  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$slice": [ -2, 3 ] } } )
  {
      "_id": {
        "$oid": "58257889ec5c9b3b7e000000"
      },
      "a": [
        4,
        5
      ]
  }
  Return 1 row(s).
  ```

  > **Note:**  
  > 子串长度不足3。

2. 与匹配符配合使用：

  匹配字段“a”，匹配要求是从第3个元素开始取值，取1个元素，且值等于3，则返回该条记录：

  ```lang-javascript
  > db.foo.bar.find( { "a": { "$slice": [ 2, 1 ], "$et": [ 3 ] } } )
  {
      "_id": {
        "$oid": "58257889ec5c9b3b7e000000"
      },
      "a": [
        1,
        2,
        3,
        4,
        5
      ]
  }
  Return 1 row(s).
  ```


