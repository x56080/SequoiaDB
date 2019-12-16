/****************************************************
@description:	queryandupdate shardingKey
         testlink cases: seqDB-12648
@modify list:
            	2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "12649";
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

function queryandupdate ()
{
	tryCatch(
		["cmd=queryandupdate",
			cl,
			'updator={$inc:{a:1, b:1}}',
			'selector={a:"",b:""}',
			'returnnew=true',
			'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'],
		[-178],
		"Error occurs in " + getFuncName() );

	/******check rest return**********//*
	var rtnExp='{ "errno": 0 }{ "a": 2, "b": 2 }';
	if(info==rtnExp)
	{
		//ok
	}
	else
	{
		println("Error occurs in "+getFuncName()+"\nrest cmd: "+str+"\nreturn: "+info+'\nexpect return: '+rtnExp);
		throw "rest return";
	}*/

	/******check count is 1**********//*
	try
	{
		var recNum=varCL.find({a:2, b:2}).count();
		if (1 != recNum)
		{
			println("Error occurs in "+getFuncName()+", expect: cl.find() return 5 record, atually: "+recNum);
			throw "check count";
		}
	}	
	catch(e)
	{
		throw e;
	}*/

}
/*
commDropCL(db,csName,clName,true,true,"drop cl in begin");

var opt={ReplSize:0, ShardingKey:{a:1}};
var varCL=commCreateCL(db,csName,clName,opt,true,false,"create cl in begin");

insertRecs();
queryandupdate();

commDropCL(db, csName, clName, false,true, "drop cl in clean in finally");
*/