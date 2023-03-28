##语法##

```lang-json
{<字段名>: {$concat: <值>}}
{<字段名>: {$concat: <位置>, [<值1>, <值2>, ...]}}
```

##说明##

$concat 用于连接字符串。当字段类型为字符串时，将直接对字符串进行连接；当字段类型为数组时，将对每个数组元素进行连接；当字段类型为非字符串时，会先将字段类型转换为字符串类型，再对字符串进行连接；当字段类型无法转换为字符串类型时，将返回 null。

SequoiaDB 支持转换的类型有 Int32、Int64、Double、Decimal、Date、Timestamp、Object、Objectid、Array 和 Boolean。

- {$concat: <值>}表示将 <值> 连接到字段的末尾，其中 <值> 不支持数组类型。

- {$concat: [<位置>, [<值1>, <值2>, ...]]}表示将字段插入至给定数组的指定位置，并与数组元素连接为字符串。

    - 正整数：将字段值插入至数组开头第 N 个元素之后。
    - 0：将字段连接至数组元素的开头。
    - 负整数：将字段值插入至数组末尾第 N 个元素之后。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "abc"})
> db.sample.employee.insert({a: ["aa", "bb"]})
```

- 作为选择符使用

    将字符串“111”连接到字段 a 的末尾并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$concat: 111}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "abc111"
    }
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        "aa111",
        "bb111"
      ]
    }
    Return 2 row(s).
    ```

    将字段 a 插入至数组开头第一个元素后面，并与数组元素连接为字符串后返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$concat: [1, [111, 222]]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "111abc222"
    }
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        "111aa222",
        "111bb222"
      ]
    }
    Return 2 row(s).
    ```

    将字段 a 插入至数组开头第一个元素后面，并与数组元素连接为字符串后返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$concat: [1, [111, 222, [333, 444]]]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "111abc222[ 333, 444 ]"
    }
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        "111aa222[ 333, 444 ]",
        "111bb222[ 333, 444 ]"
      ]
    }
    Return 2 row(s).
    ```

    > **Note:**
    >
    > 如果给定数组中存在数组类型的元素，该元素将以数组的格式进行字符串连接。

    将字段 a 插入至数组末尾第二个元素后面，并与数组元素连接为字符串后返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$concat: [-2, [111, 222, 333]]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "111222abc333"
    }
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        "111222aa333",
        "111222bb333"
      ]
    }
    Return 2 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 连接字符串后值为“abc111”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$concat: 111, $et: "abc111"}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "abc"
    }
    Return 1 row(s).
    ```