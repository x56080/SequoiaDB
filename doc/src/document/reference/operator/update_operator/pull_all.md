##语法##

```
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ], <字段名2>: [ <值1>, <值2>, ..., <值N> ], ... } }
```

##描述##

$pull_all 与 [$pull](reference/operator/update_operator/pull.md) 功能类似。

区别在于：

$pull 只能匹配某个字段的一个值，$pull_all 能匹配某个字段的多个值。

执行一次 $pull_all ，如

```
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ] } }
```

相当于执行多次 $pull

```
{ $pull: { <字段名1>: <值1> } }
{ $pull: { <字段名1>: <值2> } }
...
{ $pull: { <字段名1>: <值N> } }
```

##示例##

* 操作 arr 字段，删除数组中为 2 或者 为 3 的元素；操作 name 字段，删除数组中为 "Tom" 的元素。如有记录：

 ```
 { arr: [ 1, 2, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 ```lang-javascript
 > db.foo.bar.update( { $pull_all: { arr: [ 2, 3 ], name: [ "Tom" ] } } )
 ```

 此操作后，记录更新为：

 ```
 { arr: [ 1, 4, 5 ], age: 10, name: [ "Mike" ] }
 ```

* 操作 arr 字段，删除数组中为 4 或者 为 5 的元素。如有记录：

 ```
 { arr: [ 1, 3, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 ```lang-javascript
 > db.foo.bar.update( { $pull_all: { arr: [ 4, 5 ] } } )
 ```

 此操作后，记录更新为：

 ```
 { arr: [ 1, 3 ], age: 10, name: [ "Tom", "Mike" ] }
 ```
