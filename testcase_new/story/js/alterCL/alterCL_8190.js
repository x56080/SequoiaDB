/******************************************************************************
*@Description : 1. main-collection altered to partition-collection
*@Modify list :
*               2014-07-09 pusheng Ding  Init
*               2015-03-28 xiaojun Hu    Changed
******************************************************************************/
MAINCLNAME = CHANGEDPREFIX+"_8190_bar";
SUBCL1NAME = CHANGEDPREFIX+"_8190_sub1";
SUBCL2NAME = CHANGEDPREFIX+"_8190_sub2";
SUBCL3NAME = CHANGEDPREFIX+"_8190_sub3";

function main()
{
   db.setSessionAttr( { PreferedInstance: "M" } );
	//get ReplicaGroups
	try{
		var grouplist = Array();
		var cur = db.listReplicaGroups();
		while(cur.next()){
			if(cur.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN ){
				grouplist.push(cur.current().toObj()['GroupName']);
			}
		}
		var groups_num = grouplist.length;
	}catch(e)
	{
		println("get ReplicaGroups info fail! rc="+e);
		throw e;
	}
	println("ReplicaGroups: " + grouplist);

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
	//create main-CL
	try{
      var optionObj = {IsMainCL:true, ReplSize:0, ShardingKey:{id:1}};
      var mainCL = commCreateCLByOption( db, COMMCSNAME, MAINCLNAME, optionObj, true,
                                          false, "create main collecton failed" );
	}catch(e)
	{
		println("can't create main-CL:" + MAINCLNAME + " rc="+e);
		throw e;
	}
	println("create mainCL finished");
	
	//create sub-CL
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
		println("create subCLs fail! rc="+e);
		throw e;
	}
	println("create subCLs finished");
	
	//mainCL-noAttach altered to range-collection,expect fail
	try{
		mainCL.alter({ShardingKey:{id:1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("1 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{id:1,b:-1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("2 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{b:-1,c:1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("3 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	println("mainCL-noAttach altered to range-collection finish!");
	
	//mainCL-noAttach altered to hash-collection, expect fail
	try{
		mainCL.alter({ShardingKey:{id:1},ShardingType:'hash',Partition:4096});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("1 mainCL altered to hash-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{id:1,b:-1},ShardingType:'hash'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("2 mainCL altered to hash-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{b:-1,c:1},ShardingType:'hash',Partition:1024});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("3 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	println("mainCL-noAttach altered to hash-collection finish!");
	
	//attach sub-cl
	try{
		mainCL.attachCL(COMMCSNAME + "." + SUBCL1NAME,{LowBound:{id:-1000},UpBound:{id:0}});
		mainCL.attachCL(COMMCSNAME + "." + SUBCL2NAME,{LowBound:{id:0},UpBound:{id:1000}});
		mainCL.attachCL(COMMCSNAME + "." + SUBCL3NAME,{LowBound:{id:1000},UpBound:{id:2000}});
	}catch(e)
	{
		println("attach sub-CL fail!");
		throw e;
	}
	println("attach sub-CL finish!");
	
	//insert data
	try{
		for(var i=0;i<3000;i++){mainCL.insert({id:i-1000,b:i,c:"abcdefghijkl"+i});}
		println("insert-data succ");
	}catch(e)
	{
		println("insert-data fail! rc="+e);
		throw e;
	}
	
	//mainCL-attached altered to range-collection, expect fail
	try{
		mainCL.alter({ShardingKey:{id:1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("4 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{id:1,b:-1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("5 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{b:-1,c:1},ShardingType:'range'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("6 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	println("mainCL-attached altered to range-collection finish!");
	
	//mainCL-attached data altered to hash-collection, expect fail
	try{
		mainCL.alter({ShardingKey:{id:1},ShardingType:'hash',Partition:4096});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("4 mainCL altered to hash-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{id:1,b:-1},ShardingType:'hash'});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("5 mainCL altered to hash-collection succ,but expect fail!");
			throw e;
		}
	}
	try{
		mainCL.alter({ShardingKey:{b:-1,c:1},ShardingType:'hash',Partition:1024});
		throw 1;
	}catch(e)
	{
		if(e == 1)
		{
			println("6 mainCL altered to range-collection succ,but expect fail!");
			throw e;
		}
	}
	println("mainCL-attached altered to hash-collection finish!");
	
	//select * from bar where id=1
	//expect one record
	try{
		var sel = mainCL.find({id:{$et:1}});
		var size=0;
		var flag=false;
		while(sel.next())
		{
			size++;
			if(size>100)
				break;
			var ret = sel.current();
			if(ret.toObj()['id']==1 && ret.toObj()['b']==1001 && ret.toObj()['c']=='abcdefghijkl1001')
				flag = true;
		}
		if(size!=1)
		{
			throw 1;
		}
		if(!flag)
		{
			throw 2;
		}	
	}catch(e)
	{
		if(e==1)
			println("result-records count not expected. expect:1 return:"+size);
		else if(e==2)
		{	
			println("record not expected!");
			println("expected:{id:1,b:1001,c:'abcdefghijkl1001'}");
			println("returned:"+ret);
		}
		else
			println("select " + MAINCLNAME + " fail! rc="+e);
		throw e;
	}
	println("data-verify succ!");
	
	//detachCL
	try{
		mainCL.detachCL(COMMCSNAME + "." + SUBCL1NAME);
		mainCL.detachCL(COMMCSNAME + "." + SUBCL2NAME);
		mainCL.detachCL(COMMCSNAME + "." + SUBCL3NAME);
	}catch(e)
	{
		println("detachCL fail! rc="+e);
		throw e;
	}
	println("detach subCL succ!");
	
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
}

// Add inspect standalone run mode
try
{
   // Inspect the run mode is standalone or not
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone" ;
   main();
}
catch( e )
{
   if( "ModeStandAlone" == e )
      println( "The run mode is standalone" ) ;
   else
      throw e ;
}

