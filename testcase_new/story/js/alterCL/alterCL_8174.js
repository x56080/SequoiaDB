/******************************************************************************
*@Description : 1. collection altered to main-collection, fail
*@Modify list :
*               2014-07-10 pusheng Ding  Init
*               2015-03-28 xiaojun Hu    Changed
******************************************************************************/
CLNAME_1 = CHANGEDPREFIX+"_8174_bar1" ;
CLNAME_2 = CHANGEDPREFIX+"_8174_bar2" ;
CLNAME_3 = CHANGEDPREFIX+"_8174_bar3" ;
CLNAME_4 = CHANGEDPREFIX+"_8174_bar4" ;

try
{
   commDropCL( db, COMMCSNAME, CLNAME_1, true, true, "drop colleciton 1" );
   commDropCL( db, COMMCSNAME, CLNAME_2, true, true, "drop colleciton 2" );
   commDropCL( db, COMMCSNAME, CLNAME_3, true, true, "drop colleciton 3" );
   commDropCL( db, COMMCSNAME, CLNAME_4, true, true, "drop colleciton 4" );
}
catch( e )
{
   println( "failed to clean in the beginning" + e ) ;
   throw e ;
}

//create normalCL
try{
   var normalCL= commCreateCL( db, COMMCSNAME, CLNAME_1, -1, true, true,
                               false, "Failed to create collection" ) ;
}catch(e)
{
	println("can't create normal-CL:" + CLNAME_1 + " rc="+e);
	throw e;
}
println("create normalCL finished");

//normalCL altered to main-collection,expect fail
try{
	normalCL.alter({ShardingKey:{id:1},IsMainCL:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("normalCL altered to main-collection succ,but expect fail!");
		throw e;
	}
}
println("normalCL test finish!");

//create rangeCL
try{
   var optionObj = {ShardingKey:{a:1,b:1},ShardingType:'range',ReplSize:1};
   var rangeCL = commCreateCLByOption( db, COMMCSNAME, CLNAME_2, optionObj, true,
                                       false, "create collecton 2 failed" );
}catch(e)
{
	println("can't create rangeCL:" + CLNAME_2 + " rc="+e);
	throw e;
}
println("create rangeCL finished");

try{
	rangeCL.alter({ReplSize:0,IsMainCL:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("rangeCL altered to main-collection succ,but expect fail!");
		throw e;
	}
}
println("rangeCL test finish!");

//create hashCL
try{
   var optionObj = {ShardingKey:{b:1},ShardingType:'hash'};
   var hashCL = commCreateCLByOption( db, COMMCSNAME, CLNAME_3, optionObj, true,
                                       false, "create collecton 3 failed" );
}catch(e)
{
	println("can't create hashCL:" + CLNAME_3 + " rc="+e);
	throw e;
}
println("create hashCL finished");

try{
	hashCL.alter({IsMainCL:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("hashCL altered to main-collection succ,but expect fail!");
		throw e;
	}
}
println("hashCL test finish!");

//create mainCL
try{
   var optionObj = {ShardingKey:{b:1},IsMainCL:true};
   var mainCL = commCreateCLByOption( db, COMMCSNAME, CLNAME_4, optionObj, true,
                                       false, "create collecton 4 failed" );
}catch(e)
{
	println("can't create mainCL:" + CLNAME_4 + " rc="+e);
	throw e;
}
println("create mainCL finished");

try{
	mainCL.alter({IsMainCL:false});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("mainCL altered to normal-collection succ,but expect fail!");
		throw e;
	}
}
println("mainCL test finish!");

//clean test-env
try{
   commDropCL( db, COMMCSNAME, CLNAME_1, false, false, "drop colleciton 1" );
   commDropCL( db, COMMCSNAME, CLNAME_2, false, false, "drop colleciton 2" );
   commDropCL( db, COMMCSNAME, CLNAME_3, false, false, "drop colleciton 3" );
   commDropCL( db, COMMCSNAME, CLNAME_4, false, false, "drop colleciton 4" );
}catch(e)
{
	println("clean test-evn fail! rc="+e);
	throw e;
}
println("clean test-evn succ!");

