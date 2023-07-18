##NAME##

StreamToken - Generate position information for a data stream

##SYNOPSIS##

**StreamToken()**

**StreamToken(\<token\>)**

##CATEGORY##

StreamToken

##DESCRIPTION##

This function is used to generate a token contains position information for a data stream. It allows for reading the data stream from the latest position, or reading from a specified position.

## PARAMETERS

token (*string, optional*)

    The position information string indicating from where to start reading the data stream.

## RETURN VALUE

When the function executes successfully, it returns the generated StreamToken object.

When the function fails, an exception will be thrown and an error message will be printed.

## ERRORS

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

## VERSION

v7.2.2 and above

## EXAMPLES

Generate a token for reading the data stream from the latest position:

```lang-javascript
> var token = new StreamToken()
```

Generate a token for reading the data stream from a specified position:

```lang-javascript
> var token = StreamToken("00010000000003e800000000000000000000000000000ac80000000100000000")
```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md