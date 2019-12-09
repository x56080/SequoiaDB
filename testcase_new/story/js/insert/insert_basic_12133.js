/******************************************************************************
@Description : Insert basic: data type
@Modify list :
               2015-01-29 pusheng Ding  Init
******************************************************************************/

var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in begin" );
}
catch( e )
{
	if( e != -34 )
	{
		println( "unexpected err happened when clear cs:" + e );
		throw e;
	}
}

var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0 } );

//int32 && int64
var records = new Array();

records[0] = { a: 0, b: "zero" };
records[1] = { a: -1, b: "plus one" };
records[2] = { a: 2147483647, b: "max_int32" };
records[3] = { a: 9223372036854775807, b: "max_int64" };

for( i = 0; i < records.length; i++ )
{
	varCL.insert( records[i] );
}

var cursor = varCL.find();

for( i = 0; i < records.length; i++ )
{
	while( cursor.next() )
	{
		record = cursor.current();
		recordObj = record.toObj();
		recordStr = record.toJson();

		if( recordObj["b"] == records[i]["b"] )
		{
			if( recordObj["a"] != records[i]["a"] )
			{
				println( "the records " + recordStr + " is not match insert records" );
				throw -1;
			}
			else
			{
				break;
			}
		}

	}
	cursor = varCL.find();
}

//float
try
{
	var typeFloat = 123e+50;
	varCL.remove();
	varCL.insert( { a: typeFloat, b: "float" } );
	var sel = varCL.find( { b: "float" } );
	if( sel.next() )
	{
		var ret = sel.current();
		if( ret.toObj()['a'] != typeFloat )
		{
			throw "float-verify-failed";
		}
	}
	sel.close();

} catch( e )
{
	if( e == "float-verify-failed" )
	{
		println( "expect:" + typeFloat + " \treturn:" + ret.toObj()['a'] );
	}
	println( "test insert-float failed!" );
	throw e;
}

//string
try
{
	var typeString = "value:?*";
	varCL.remove();
	varCL.insert( { a: typeString, b: "string" } );
	var sel = varCL.find( { b: "string" } );
	if( sel.next() )
	{
		var ret = sel.current();
		if( ret.toObj()['a'] != typeString )
		{
			throw "String-verify-failed";
		}
	}
	sel.close();

} catch( e )
{
	if( e == "String-verify-failed" )
	{
		println( "expect:" + typeString + " \treturn:" + ret.toObj()['a'] );
	}
	println( "test insert-string failed!" );
	throw e;
}

//date && timestamp
try
{
	var insDate = "2015-01-29";
	varCL.insert( { a: { "$date": insDate }, b: "date" } );
	var insTimestamp = "2015-01-29-14.30.40.124233";
	varCL.insert( { a: { "$timestamp": insTimestamp }, b: "timestamp" } );
	var sel = varCL.find( { b: "date" } );
	if( sel.next().toObj()['a']['$date'] != insDate )
		throw "Date verify failed";
	var sel = varCL.find( { b: "timestamp" } );
	if( sel.next().toObj()['a']['$timestamp'] != insTimestamp )
		throw "Timestamp verify failed";
	sel.close();
} catch( e )
{
	println( "test insert-date&&timestamp failed!" );
	throw e;
}

//OID && binary && regex
try
{
	var oidStr = "123abcd00ef12358902300ef";
	var binaryStr = "aGVsbG8gd29ybGQ=";
	var regexStr = "HelloWorld";
	varCL.insert( { a: { "$oid": oidStr }, b: "OID" } );
	varCL.insert( { a: { "$binary": binaryStr, "$type": "1" }, b: "binary" } );
	varCL.insert( { a: { "$regex": regexStr, "$options": "i" }, b: "regex" } );
	var cur = varCL.find( { b: "OID" } );
	if( cur.next().toObj()['a']['$oid'] != oidStr )
		throw "oid verify failed";
	var cur = varCL.find( { b: "binary" } );
	if( cur.next().toObj()['a']['$binary'] != binaryStr )
		throw "binary verify failed";
	var cur = varCL.find( { b: "regex" } );
	if( cur.next().toObj()['a']['$regex'] != regexStr )
		throw "regex verify failed";
	cur.close();
} catch( e )
{
	println( "test insert-OID&&binary&&regex failed!" );
	throw e;
}

//bool
try
{
	varCL.insert( { a: true, b: "booleantrue" } );
	varCL.insert( { a: false, b: "booleanfalse" } );
	var cur = varCL.find( { b: "booleantrue" } );
	if( cur.next().toObj()['a'] != true )
		throw "boolean true verify failed"
	var cur = varCL.find( { b: "booleanfalse" } );
	if( cur.next().toObj()['a'] != false )
		throw "boolean false verify failed"
	cur.close();
} catch( e )
{
	println( "test insert-bool failed!" );
	throw e;
}

//object
try
{
	var insObj = { a: { a: 1, b: "one" }, b: "object" };
	varCL.insert( insObj );
	var cur = varCL.find( { b: "object" } );
	if( !compareObj( insObj, cur.next().toObj() ) )
		throw "object verify failed"
	cur.close();
} catch( e )
{
	println( "test insert-object failed!" );
	throw e;
}

//array
try
{
	var ins1 = { a: ["abc", 0, "def"], b: "array1" };
	var ins2 = { a: [{ a1: "array", b1: [1, 2, 3] }, "type array", 123], b: "array2" };
	varCL.insert( ins1 );
	varCL.insert( ins2 );
	var cur = varCL.find( { b: "array1" } );
	if( !compareObj( ins1, cur.next().toObj() ) )
		throw "array1 verify failed";
	var cur = varCL.find( { b: "array2" } );
	if( !compareObj( ins2, cur.next().toObj() ) )
		throw "array2 verify failed"
	cur.close();
} catch( e )
{
	println( "test insert-array failed!" );
	throw e;
}

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in end" );
}
catch( e )
{
	println( "failed to drop cs, rc= " + e );
	throw e;
}
