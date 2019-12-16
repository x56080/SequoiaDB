/****************************************************
@description:	update shardingKey
         testlink cases: seqDB-12648
@modify list:
            	2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "12648";

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

function updateAndCheck ()
{
	var word = "update";
	tryCatch(
		["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'],
		[-178],
		"updateAndCheck: fail to run rest command: " + word );
	/******check count is 1**********//*
	try
	{
		var size = varCL.count({a:2, b:2});
		if (1 != size)
		{
			throw "rest [cmd=update] change the record number to "+size;
		}
	}	
	catch(e)
	{
		throw e;
	}*/
}
/*
commDropCL(db,csName,clName,true,true,"drop cl in begin");

var opt = {ReplSize:0, ShardingKey:{a:1}};
var varCL = commCreateCL(db,csName,clName,opt,true,false,"create cl in begin");

insertRecs();
updateAndCheck();

commDropCL(db, csName, clName, false,false, "drop cl in clean");
*/