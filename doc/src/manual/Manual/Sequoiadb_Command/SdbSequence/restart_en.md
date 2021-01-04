##NAME##

restart - restart the sequence

##SYNOPSIS##

**sequence.restart\(\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Restart the sequence. The sequence value will be set back to the start.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, return void.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Restart a sequence with the current value of 10

```lang-javascript
> sequence.getNextValue()
10
> sequence.restart()
```

Then the sequence value goes back to the start value of 1

```lang-javascript
> sequence.getNextValue()
1
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
