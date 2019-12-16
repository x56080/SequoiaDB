/****************************************************
@description:	queryandupdate,normal case
         testlink cases: seqDB-7207
         queryandupdate(): use only the required parameters
@input:		1 insert{myid:229095,age:9}
				2 cmd=queryandupate name=cs.cl updator={$inc:{age:1}}	
@expectation:	cl has 1 record: {myid:229095,age:10}
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

function insertRecs ()
{
	try
	{
		varCL.insert( { myid: 229095, age: 9 } );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function queryandupdate ()
{
	tryCatch(
		["cmd=queryandupdate", cl, 'updator={$inc:{age:1}}'],
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
		var recNum1 = varCL.find( { "myid": 229095, "age": 10 } ).count();
		if( 1 != recNum1 )
		{
			println( "Error occurs in " + getFuncName() + ', expect: cl.find({"myid": 229095,"age": 10}) return 1 record, atually: ' + recNum1 );
			throw 'check rec: {"myid": 229095,"age": 10}';
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
	insertRecs();
	queryandupdate();
}
catch( e )
{
	throw e;
}
finally
{
	commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}