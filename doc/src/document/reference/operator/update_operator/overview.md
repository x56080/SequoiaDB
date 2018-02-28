##更新符##

更新符可以实现对字段的添加、修改、删除操作。支持的更新符如下：

| 更新符 | 描述 | 示例 |
|--------|------|------|
| [$inc](reference/operator/update_operator/inc.md) | 增加指定字段的值 | db.foo.bar.update({ $inc: { age: 5, ID: 1 } }, { age: { $gt: 15 } }) |
| [$set](reference/operator/update_operator/set.md) | 将指定字段更新为指定的值 | db.foo.bar.update({ $set: { str: "abd" } }) |
| [$unset](reference/operator/update_operator/unset.md) | 删除指定的字段 | db.foo.bar.update({ $unset: { name: "", age: "" } }) |
| [$addtoset](reference/operator/update_operator/addtoset.md) | 向数组中添加元素和值 | db.foo.bar.update({ $addtoset: { arr: [1,3,5] } }, { arr: { $exists: 1 } }) |
| [$pop](reference/operator/update_operator/pop.md) | 删除指定数组中的最后N个元素 | db.foo.bar.update({ $pop: { arr: 2 } }) |
| [$pull](reference/operator/update_operator/pull.md)<br>[$pull_by](reference/operator/update_operator/pull_by.md) | 清除指定数组中的指定值 | db.foo.bar.update({ $pull: {arr: 2, name: "Tom" } })<br>db.foo.bar.update({ $pull_by: {arr: 2, name: "Tom" } }) |
| [$pull_all](reference/operator/update_operator/pull_all.md)<br>[$pull_all_by](reference/operator/update_operator/pull_all_by.md) | 清除指定数组中的指定值 | db.foo.bar.update({ $pull_all: { arr: [2,3], name: ["Tom"] } })<br>db.foo.bar.update({ $pull_all_by: { arr: [2,3], name: ["Tom"] } }) |
| [$push](reference/operator/update_operator/push.md) | 将给定值插入到数组中 | db.foo.bar.update({ $push: { arr: 1 } }) |
| [$push_all](reference/operator/update_operator/push_all.md) | 向指定数组中插入所有给定值 | db.foo.bar.update({ $push_all: { arr: [1,2,8,9] } }) |
| [$replace](reference/operator/update_operator/replace.md) | 将文档全部替换 | db.foo.bar.update({ $replace: { age: 0, name: 'default' } }, { age: { $exists: 0 } }) |
