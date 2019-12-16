/****************************************************
@description:	upsert,normal case
         testlink cases: seqDB-7216
@input:		1 cmd=upsert&name=foo.bar&updator={$inc:{age:1}}
@expectation:	cl has 1 record: {age:1}
@modify list:
            	2015-5-25 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
var cl = "name=" + csName + '.' + clName;
var varCL;

function ready ()
{
	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
	var index = { age: 1 };
	commCreateIndex( varCL, "ageIndex", index, false, false );
}

function upsert ()
{
	tryCatch(
		["cmd=upsert", cl, 'updator={$inc:{age:1}}'],
		[0],
		"Error occurs in " + getFuncName() );

	/******check count is 1**********/
	try
	{
		var recNum = varCL.find().count();
		if( 1 != recNum )
		{
			println( "Error occurs in " + getFuncName() + ", expect: cl.find() return 1 record, atually: " + recNum );
			throw "check count";
		}
	}
	catch( e )
	{
		throw e;
	}

	/******check record in cl**********/
	try
	{
		var recNum1 = varCL.find( { "age": 1 } ).count();
		if( 1 != recNum1 )
		{
			println( "Error occurs in " + getFuncName() + ', expect: cl.find({"age": 1}) return 1 record, atually: ' + recNum1 );
			throw 'check rec: {"age": 1}';
		}
	}
	catch( e )
	{
		throw e;
	}

}

try
{
	ready();
	upsert();
}
catch( e )
{
	throw e;
}
finally
{
	commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}