/****************************************************
@description:	ssl, normal case
         testlink cases: seqDB-9654
@input:		connect sdb by ssl, then create cl
@expectation:	return errno:0
@modify list:
            	2017-11-21 XiaoNi Huang init
****************************************************/
var csName =COMMCSNAME;
var clName =COMMCLNAME+"9654";
var cs ="name="+csName;
var cl ="name="+csName+'.'+clName;

function createclAndCheck(){
	var word="create collection";
	tryCatchByHttps(
		["cmd="+word,cl,"options={ShardingKey:{age:1}}"] ,
		[0] ,
		"Fail to run rest [cmd="+word+"]");
	
	try
	{
      db.getCS(csName).getCL(clName);
	}
	catch(e)
	{
		println( "Error: rest [cmd="+word+"] has been done, but cl["+cl+"] does not exist");
		throw e;
	}
	
	//check cl attribute
   var tep=db.snapshot(8,{Name:csName+'.'+clName}).current().toObj()["ShardingKey"];
   if ( JSON.stringify(tep)!='{"age":1}' ){
   throw 'Error: options parameter Shardingkey, expect: {"age":1}, actual: '+JSON.stringify(tep);
   }	
}

function dropclAndCheck(){
	var word="drop collection";
	tryCatchByHttps(
		["cmd="+word,cl] ,
		[0] ,
		"Fail to run rest [cmd="+word+"]");
	
	try
	{	
		var flag=false;
	      db.getCS(csName).getCL(clName);
		flag=true;
	}
	catch ( e )
	{
		if(e!=-23)
		{
			println("Error: rest [cmd=drop collection]  has been done, cs.getCL("+clName+"), expect: return -23, actually: return "+e);
			throw e;
		}	
	}

	if (true==flag)
	{
		db.getCS(csName).dropCL(clName);
		throw "Error: rest [cmd=drop collection]  has been done, cs.getCL("+clName+"), expect: return -23, actually: return 0";
	}

}

if(true==commIsStandalone(db))
{
	println("Mode is standalone!");
}
else
{
   commDropCL(db, csName, clName, true, true,"drop cl in begin");
   createclAndCheck();
   dropclAndCheck();
}