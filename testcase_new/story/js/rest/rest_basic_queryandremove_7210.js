/****************************************************
@description:	queryandremove,normal case
         testlink cases: seqDB-7210
         queryandremove(): use only the required parameters
@input:		1 insert{myid:229095,age:10}
				2 cmd=queryandremove&name=cs.cl
@expectation:	cl has no record
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
	varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
	var index = { age: 1 };
	commCreateIndex( varCL, "ageIndex", index, false, false );
}

function insertRecs ()
{
	try
	{
		varCL.insert( { _id: 229095, age: 10 } );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function queryandremove ()
{
	tryCatch(
		["cmd=queryandremove", cl],
		[0],
		"Error occurs in " + getFuncName() );

	/******check rest return**********/
	var rtnExp = '{ "errno": 0 }{ "_id": 229095, "age": 10 }';
	if( info == rtnExp )
	{
		//ok
	}
	else
	{
		println( "Error occurs in " + getFuncName() + "\nrest cmd: " + str + "\nreturn: " + info + '\nexpect return: ' + rtnExp );
		throw "rest return";
	}

	/******check count is 0**********/
	try
	{
		var recNum = varCL.find().count();
		if( 0 != recNum )
		{
			println( "Error occurs in " + getFuncName() + ", expect: cl.find() return 0 record, atually: " + recNum );
			throw "check count";
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
	queryandremove();
}
catch( e )
{
	throw e;
}
finally
{
	commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}