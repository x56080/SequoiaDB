/******************************************************************************
@Description : 1. collection altered Compress, fail
@Modify list :
               2014-07-10 pusheng Ding  Init
******************************************************************************/
CLNAME_1 = CHANGEDPREFIX+"_8175_bar1" ;
CLNAME_2 = CHANGEDPREFIX+"_8175_bar2" ;
CLNAME_3 = CHANGEDPREFIX+"_8175_bar3" ;
CLNAME_4 = CHANGEDPREFIX+"_8175_bar4" ;

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
   var optionObj = {ReplSize:0,Compressed:true};
   var normalCL = commCreateCLByOption( db, COMMCSNAME, CLNAME_1, optionObj, true,
                                       false, "create collecton 1 failed" );
}catch(e)
{
	println("can't create normal-CL:" + CSPREFIX_CL1 + " rc="+e);
	throw e;
}
println("create normalCL finished");

//Compressed True to False,expect fail
try{
	normalCL.alter({ShardingKey:{id:1},ShardingType:'hash',Compressed:false});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("normalCL alters Compressed succ,but expect fail!");
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
	println("can't create rangeCL:" + CSPREFIX_CL2 + " rc="+e);
	throw e;
}
println("create rangeCL finished");

try{
	rangeCL.alter({Compressed:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("rangeCL alters Compressed succ,but expect fail!");
		throw e;
	}
}
println("rangeCL test finish!");

//create hashCL
try{
   var optionObj = {ShardingKey:{b:1},ShardingType:'hash',Compressed:true};
   var hashCL = commCreateCLByOption( db, COMMCSNAME, CLNAME_3, optionObj, true,
                                       false, "create collecton 3 failed" );
}catch(e)
{
	println("can't create hashCL:" + CSPREFIX_CL3 + " rc="+e);
	throw e;
}
println("create hashCL finished");

try{
	hashCL.alter({Compressed:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("hashCL alters Compressed succ,but expect fail!");
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
	println("can't create mainCL:" + CSPREFIX_CL4 + " rc="+e);
	throw e;
}
println("create mainCL finished");

try{
	mainCL.alter({Compressed:true});
	throw 1;
}catch(e)
{
	if(e == 1)
	{
		println("mainCL alters Compressed succ,but expect fail!");
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

