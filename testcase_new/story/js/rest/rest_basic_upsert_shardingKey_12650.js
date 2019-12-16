/****************************************************
@description:	upsert shardingKey
         testlink cases: seqDB-12650
@modify list:
            	2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "12650";
var cl = "name=" + csName + '.' + clName;

function insertRecs ()
{
	try
	{
		varCL.insert( { a: 1, b: 1 } );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function upsert ()
{
	tryCatch(
		["cmd=upsert",
			cl,
			'updator={$inc:{a:1, b:1}}',
			'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'],
		[-178],
		"Error occurs in " + getFuncName() );

	/******check count is 1**********/
	/*
	try
	{
		var recNum=varCL.find({a:2, b:2}).count();
		if (1 != recNum)
		{
			println("Error occurs in "+getFuncName()+", expect: cl.find() return 1 record, atually: "+recNum);
			throw "check count";
		}
	}	
	catch(e)
	{
		throw e;
	}
	*/
}
/*
commDropCL(db,csName,clName,true,true,"drop cl in begin");

var opt={ReplSize:0, ShardingKey:{a:1}};
var varCL=commCreateCL(db,csName,clName,opt,true,false,"create cl in begin");

insertRecs();
upsert();

commDropCL(db, csName, clName, false,true, "drop cl in clean in finally");
*/