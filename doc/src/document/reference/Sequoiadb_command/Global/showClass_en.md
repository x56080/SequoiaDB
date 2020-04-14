##NAME##

showClass - List all the custom Classes built-in the Sdb Shell and all methods for those custom Classes.

##SYNOPSIS##

**showClass([className])**

##CATEGORY##

Global

##DESCRIPTION##

This method is used to list all the built-in custom Classes supported by the Sdb Shell or list all the methods contained in the specified custom Classes.

##PARAMETERS##

* `className` ( *String*， *Optional* )

   Class name to be enumerated.

##RETURN VALUE##

If the className is empty, it will return all the built-in custom Class supported by the Sdb Shell;
Conversely, return all the methods contained in the specified built-in custom Class.

##HISTORY##

Since v2.8

##EXAMPLES##

1. List all the built-in custom Classes supported by Sdb Shell.

	```lang-javascript
	> showClass()
	All classes:
   	   BSONArray
   	   BSONObj
   	   BinData
   	   CLCount
   	   Cmd
   	   ...
	Global functions:
       catPath()
       forceGC()
       getExePath()
       getLastErrMsg()
       ...
	Takes 0.000518s.
	```

2. List all methods contained in Class SdbDate.

	```lang-javascript
	> showClass("SdbDate")
	SdbDate's static functions:
	   help()
	SdbDate's member functions:
   	   help()
       toString()
	Takes 0.000218s.
	```

