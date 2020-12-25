##名称##

setCurrentValue - 设置序列的当前值

##语法##

**sequence.setCurrentValue\(\<value\>\)**

##描述##

该函数用设置序列的当前值，从而调整序列的进度。与 [setAttributes()][setAttributes] 接口中设置 CurrentValue 属性不同，该函数不允许反向设置当前值。对于递增序列，当前值只能增大，不能减小；对于递减序列，则相反。这个特性可以避免序列生成重复的值。

##参数##

value（ *number*， *必填* ）

指定的当前值

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setCurrentValue()`函数常见异常如下：

| 错误码 | 错误类型                | 可能发生的原因       | 解决办法 |
| ------ | ----------------------- | -------------------- | -------- |
| -361   | SDB_SEQUENCE_VALUE_USED | 指定当前值已经被使用 | 如要反向设置，可以使用 [setAttributes()][setAttributes] 设置 CurrentValue 属性。 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

- 将递增序列值设置为更小的值

   ```lang-javascript
   > var sequence = db.createSequence( "IDSequence" )
   > sequence.getCurrentValue()
   1000
   > sequence.setCurrentValue( 500 )
   ```

   输出错误信息如下：

   ```lang-text
   (shell):1 uncaught exception: -361
   Sequence value has been used
   ```

- 将递增序列值设置为更大的值

   ```lang-javascript
   > sequence.getCurrentValue()
   1000
   > sequence.setCurrentValue( 2000 )
   ```

   设置后获取的当前序列值如下：
  
    ```lang-javascript
   > sequence.getCurrentValue()
   2000
   ```


[^_^]:
     本文使用的所有引用及链接
[setAttributes]:reference/Sequoiadb_command/SdbSequence/setAttributes.md
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/faq.md
[error_code]:manual/reference/sequoiadb_error_code.md
