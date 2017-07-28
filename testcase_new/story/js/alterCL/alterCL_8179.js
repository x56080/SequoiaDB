/******************************************************************************
*@Description : 1. main collection alters replsize
*@Modify list :
*               2014-07-04  pusheng Ding  Init
*               2015-03-28 xiaojun Hu    Changed
******************************************************************************/
MAINCLNAME = CHANGEDPREFIX+"_8179_main" ;
SUBCL1NAME = CHANGEDPREFIX+"_8179_sub1";
SUBCL2NAME = CHANGEDPREFIX+"_8179_sub2";
SUBCL3NAME = CHANGEDPREFIX+"_8179_sub3";

try{
   db.setSessionAttr( { PreferedInstance: "M" } );
	var isStandalone = commIsStandalone( db ) ;
}catch(e)
{
	println("can't connect to db");
	throw e;
}

try
{
   commDropCL( db, COMMCSNAME, SUBCL1NAME, true, true, "drop sub colleciton 1" );
   commDropCL( db, COMMCSNAME, SUBCL2NAME, true, true, "drop sub colleciton 2" );
   commDropCL( db, COMMCSNAME, SUBCL3NAME, true, true, "drop sub colleciton 3" );
   commDropCL( db, COMMCSNAME, MAINCLNAME, true, true, "drop main colleciton" );
}
catch( e )
{
   println( "failed to clean in the beginning" + e ) ;
   throw e ;
}

//create main-cl
try{
   var optionObj = {ShardingKey:{id:1} , ReplSize:2,Compressed:true,IsMainCL:true};
   var mainCL = commCreateCLByOption( db, COMMCSNAME, MAINCLNAME, optionObj, true,
                                       false, "create main collecton failed" );
}catch(e)
{
	println("can't create main-CL:" + MAINCLNAME + " rc="+e);
	throw e;
}
println("createCL " + MAINCLNAME + " finished");

//mainCL-noSubCL alters replsize
try{
	mainCL.alter({ReplSize:2});
	var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + MAINCLNAME});
	var replsize = sn1.current().toObj()['ReplSize'];
	if(replsize == 2)
	{
		println("mainCL-noSubCL alters replsize succ!");
	}
	else
	{
		println("mainCL-noSubCL alters replsize fail! ReplSize=" + replsize);
		throw -1;
	}	
}catch(e)
{
	if(!isStandalone)
	{
		if(e != -1)
		{
			println("mainCL-noSubCL alters replsize fail! rc="+e);
		}
		throw e;
	}
	else
	{
		println("mainCL-noSubCL alters replsize fail when standalone");
	}
}
println("mainCL-noSubCL alters replsize finish!");

//create sub-cl
try{
   var optionObj1 = {ShardingKey:{b:1},ShardingType:"hash",Partition:4096};
   var subCL1 = commCreateCLByOption( db, COMMCSNAME, SUBCL1NAME, optionObj1, true,
                                       false, "create sub collecton 1 failed" );
   var optionObj2 = {ReplSize:2};
   var subCL2 = commCreateCLByOption( db, COMMCSNAME, SUBCL2NAME, optionObj2, true,
                                       false, "create sub collecton 2 failed" );
   var optionObj3 = {Compressed:false};
   var subCL3 = commCreateCLByOption( db, COMMCSNAME, SUBCL3NAME, optionObj3, true,
                                       false, "create sub collecton 3 failed" );
}catch(e)
{
	println("can't create sub-CL");
	throw e;
}

//attach sub-cl
try{
	mainCL.attachCL(COMMCSNAME + "." + SUBCL1NAME,{LowBound:{id:-1000},UpBound:{id:0}});
	mainCL.attachCL(COMMCSNAME + "." + SUBCL2NAME,{LowBound:{id:0},UpBound:{id:1000}});
	mainCL.attachCL(COMMCSNAME + "." + SUBCL3NAME,{LowBound:{id:1000},UpBound:{id:2000}});
}catch(e)
{
	if(!isStandalone)
	{
		println("attach sub-CL fail!");
		throw e;
	}
}
println("attach sub-CL finish!");

//mainCL-SubCL alters replsize
try{
	mainCL.alter({ReplSize:3});
	var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + MAINCLNAME});
	var replsize = sn1.current().toObj()['ReplSize'];
	if(replsize == 3)
	{
		println("mainCL-SubCL alters replsize succ!");
	}
	else
	{
		println("mainCL-SubCL alters replsize fail! ReplSize=" + replsize);
		throw -1;
	}	
}catch(e)
{
	if(!isStandalone)
	{
		if(e != -1)
		{
			println("mainCL-SubCL alters replsize fail! rc="+e);
		}
		throw e;
	}
	else
	{
		println("mainCL-noSubCL alters replsize fail when standalone");
	}
}
println("mainCL-SubCL alters replsize finish!");

//insert data
try{
	for(var i=0;i<3000;i++){mainCL.insert({id:i-1000,b:i,c:"abcdefghijkl"+i});}
}catch(e)
{
	println("insert-data into mainCL fail! rc="+e);
}
println("insert-data into mainCL succ!");

//detachCL
try{
	mainCL.detachCL(COMMCSNAME + "." + SUBCL1NAME);
	mainCL.detachCL(COMMCSNAME + "." + SUBCL2NAME);
	mainCL.detachCL(COMMCSNAME + "." + SUBCL3NAME);
	println("detach subCL succ!");
}catch(e)
{
	if(!isStandalone)
	{
		println("detachCL fail! rc="+e);
		throw e;
	}
}

//clean test-env
try{
   commDropCL( db, COMMCSNAME, SUBCL1NAME, false, false, "drop sub colleciton 1" );
   commDropCL( db, COMMCSNAME, SUBCL2NAME, false, false, "drop sub colleciton 2" );
   commDropCL( db, COMMCSNAME, SUBCL3NAME, false, false, "drop sub colleciton 3" );
   commDropCL( db, COMMCSNAME, MAINCLNAME, false, false, "drop main colleciton" );
}catch(e)
{
	println("clean test-evn fail! rc="+e);
	throw e;
}
println("clean test-evn succ!");
