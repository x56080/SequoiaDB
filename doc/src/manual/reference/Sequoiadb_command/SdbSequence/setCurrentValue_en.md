##NAME##

setCurrentValue - set the current value of the sequence

##SYNOPSIS##

**sequence.setCurrentValue\(\<value\>\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Set the current value of the sequence, to adjust the progress. It is different from setting CurrentValue by [setAttributes()][setAttributes] that this function doesn't allow setting the value back. For an ascending sequence, the current value can only increase, not decrease. Descending sequences are the opposite. This feature can avoid duplicate values to be generated.

##PARAMETERS##

value ( *number*, *required* )

The current value to be set.

##RETURN VALUE##

When the function executes successfully, return void.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getCurrentValue()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-358      |SDB_SEQUENCE_VALUE_USED|Sequence value has been used|To set it back, use interface [setAttributes()][setAttributes]|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

- Decrease the current value.

    ```lang-javascript
    > sequence.getCurrentValue()
    1000
    > sequence.setCurrentValue( 500 )
    ```

    Error message:

    ```lang-text
    (shell):1 uncaught exception: -358
    Sequence value has been used
    ```

- Increase the current value.

    ```lang-javascript
    > sequence.getCurrentValue()
    1000
    > sequence.setCurrentValue( 2000 )
    ```

    Current value after setting:

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
