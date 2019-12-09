/****************************************************
@description:	queryandremove,normal case
         testlink cases: seqDB-7209
         queryandremove(): cover all parameters
@input:		1 insertmyid:229095,age:10},{myid:229098,age:13},{myid:229096,age:11,male:true},{age:12},{myid:229099,age:14}
				2 cmd=queryandremove&name=cs.cl&sort={age:1}&selector={myid:"",age:""}&skip=1&returnnum=2&filter={myid:{$exists:1}}
@expectation:	delete 229096 229098 records
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
		varCL.insert( [
			{ myid: 229095, age: 10 },
			{ myid: 229098, age: 13 },
			{ myid: 229096, age: 11, male: true },
			{ age: 12 },
			{ myid: 229099, age: 14 }
		] );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function queryandremove ()
{
	tryCatch(
		["cmd=queryandremove",
			cl,
			'sort={age:1}',
			'selector={myid:"",age:""}',
			'skip=1',
			'returnnum=2',
			'filter={myid:{$exists:1}}'],
		[0],
		"Error occurs in " + getFuncName() );

	/******check rest return**********/
	var rtnExp = '{ "errno": 0 }{ "myid": 229096, "age": 11 }{ "myid": 229098, "age": 13 }';
	if( info == rtnExp )
	{
		//ok
	}
	else
	{
		println( "Error occurs in " + getFuncName() + "\nrest cmd: " + str + "\nreturn: " + info + '\nexpect return: ' + rtnExp );
		throw "rest return";
	}

	/******check count is 3**********/
	try
	{
		var recNum = varCL.find().count();
		if( 3 != recNum )
		{
			println( "Error occurs in " + getFuncName() + ", expect: cl.find() return 3 record, atually: " + recNum );
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
		var recNum2 = varCL.find( { "age": 12 } ).count();
		var recNum3 = varCL.find( { "myid": 229099, "age": 14 } ).count();
		if( 1 != recNum1 )
		{
			println( "Error occurs in " + getFuncName() + ', expect: cl.find({"myid": 229095,"age": 10}) return 1 record, atually: ' + recNum1 );
			throw 'check rec: {"myid": 229095,"age": 10}';
		}
		if( 1 != recNum2 )
		{
			println( "Error occurs in " + getFuncName() + ', expect: cl.find({"myid": 229095,"age": 10}) return 1 record, atually: ' + recNum2 );
			throw 'check rec: {"age": 12}';
		}
		if( 1 != recNum3 )
		{
			println( "Error occurs in " + getFuncName() + ', expect: cl.find({"myid": 229095,"age": 10}) return 1 record, atually: ' + recNum3 );
			throw 'check rec: {"myid": 229099,"age": 14}';
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