##语法##

返回从“<起始下标>”开始到数组末尾的元素：

```
{ <字段名>: { $returnMatch: <起始下标> } }
```

返回从“<起始下标>”开始，长度为指定“<长度>”的元素：

```
{ <字段名>: { $returnMatch: [ <起始下标>, <长度> ] } }
```

##描述##

$returnMatch 选取匹配的数组元素，可以通过参数选取指定的元素。必须与匹配符结合使用。

##示例##

原始记录：

```lang-javascript
> db.foo.bar.find()
{
  "a": [
    1,
    2,
    4,
    7,
    9
  ]
}
```

* 查询集合 foo.bar 中“a”字段数组元素的值为1或4或7的记录：

  ```lang-javascript
  > db.foo.bar.find({a:{$in:[1,4,7]}})
  {
      "a": [
        1,
        2,
        4,
        7,
        9
      ]
  }
  ```

  > **Note:**
  > 不带$returnMatch，默认返回原始记录，这其中包括没有匹配到的元素2和9。

* 查询集合 foo.bar 中“a”字段数组元素的值为1或4或7的记录，并选取命中的所有元素：

  ```lang-javascript
  > db.foo.bar.find({a:{$returnMatch:0, $in:[1,4,7]}})
  {
      "a": [
        1,
        4,
        7
      ]
  }
  ```

  > **Note:**
  > $returnMatch:0, 表示选取下标为0开始的所有命中的元素，不包括没有命中的元素2和9

* 查询集合 foo.bar 中“a”字段数组元素的值为1或4或7的记录，并选取第2，3个命中的元素：

  ```lang-javascript
  > db.foo.bar.find({a:{$returnMatch:[1,2], $in:[1,4,7]}})
  {
      "a": [
        4,
        7
      ]
  }
  ```

  > **Note:**
  > $returnMatch:[1,2], 表示选取下标为1开始，长度为2的所有命中的元素

