/******************************************************************************
*@Description : 1. sub-collection with data altered to partition-collection
*@Modify list :
*               2014-07-08  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
******************************************************************************/

MAINCLNAME = CHANGEDPREFIX+"bar" ;
SUBCL1NAME = CHANGEDPREFIX+"_sub1";
SUBCL2NAME = CHANGEDPREFIX+"_sub2";
SUBCL3NAME = CHANGEDPREFIX+"_sub3";

function main()
{
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
      var optionObj = {IsMainCL:true,ShardingKey:{id:1}};
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
		var sn2 = db.snapshot(8,{Name:COMMCSNAME+"."+SUBCL2NAME});
		var sourceGroup2 = sn2.current().toObj()['CataInfo'][0]['GroupName'];
      var optionObj3 = {Compressed:false} ;
      var subCL3 = commCreateCLByOption( db, COMMCSNAME, SUBCL3NAME, optionObj3, true,
                                         false, "create sub collecton 3 failed" );
		var sn3 = db.snapshot(8,{Name:COMMCSNAME+"."+SUBCL3NAME});
		var sourceGroup3 = sn3.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("create subCLs fail! rc="+e);
		throw e;
	}
	println("create subCLs finished");
	
	//attach sub-cl
	try{
		mainCL.attachCL(COMMCSNAME + "." + SUBCL1NAME,{LowBound:{id:{$minKey:1}},UpBound:{id:0}});
		mainCL.attachCL(COMMCSNAME + "." + SUBCL2NAME,{LowBound:{id:0},UpBound:{id:10000}});
		mainCL.attachCL(COMMCSNAME + "." + SUBCL3NAME,{LowBound:{id:10000},UpBound:{id:{$maxKey:1}}});
	}catch(e)
	{
		println("attach sub-CL fail!");
		throw e;
	}
	println("attach sub-CL finish!");
	
	//insert data
	try{
		for(var i=0;i<30000;i++){mainCL.insert({id:i-10000,b:i,c:"abcdefghijkl"+i});}
		println("insert-data succ");
	}catch(e)
	{
		println("insert-data fail! rc="+e);
		throw e;
	}
	
	//alter subCL2 to range-collection and split
	try{
		subCL2.alter({ShardingKey:{b:1},ShardingType:'range',ReplSize:1});
		if(groups_num>1)
		{
			var tarGroupIndex=-1;
			var stepId = 2000;
			var partId = 5;
			var lowId = 0;
			var highId = 0;
			for(var i=0;i<partId;i++){
				tarGroupIndex++;
				if(tarGroupIndex == groups_num)
					tarGroupIndex=0;
				if(grouplist[tarGroupIndex]==sourceGroup2)
				{
					i--;
					continue;
				}
				lowId = (i-1)*stepId + 10000;
				highId = i*stepId + 10000;
				subCL2.split(sourceGroup2, grouplist[tarGroupIndex],{b:lowId},{b:highId});
				println(SUBCL2NAME+" split from "+sourceGroup2+" to "+ grouplist[tarGroupIndex]+" {b:"+lowId+"} {b:"+highId+"}");
			}
		}
	}catch(e)
	{
		println("alter " + SUBCL2NAME + " to range-collection and split fail! rc="+e);
		throw e;
	}
	println("alter " + SUBCL2NAME + " to range-collection and split finish!");
	
	//alter subCL3 to hash-collection and split
	try{
		subCL3.alter({ShardingKey:{id:1},ShardingType:'hash',Partition:1024});
		if(groups_num>1)
		{
			tarGroupIndex=-1;
			var stepPar = 256;
			var partId = 3;
			var lowPar = 0;
			var highPar = 0;
			for(var i=0;i<partId;i++){
				tarGroupIndex++;
				if(tarGroupIndex == groups_num)
					tarGroupIndex=0;
				if(grouplist[tarGroupIndex]==sourceGroup3)
				{
					i--;
					continue;
				}
				lowPar = i*stepPar;
				highPar = (i+1)*stepPar;
				subCL3.split(sourceGroup3, grouplist[tarGroupIndex],{Partition:lowPar},{Partition:highPar});
				println(SUBCL3NAME+" split from "+sourceGroup3+" to "+ grouplist[tarGroupIndex]+" {Partition:"+lowPar+"} {Partition:"+highPar+"}");
			}
		}
	}catch(e)
	{
		println("alter " + SUBCL3NAME + " to hash-collection and split fail! rc="+e);
		throw e;
	}
	println("alter " + SUBCL3NAME + " to hash-collection and split finish!");
	
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
			if(ret.toObj()['id']==1 && ret.toObj()['b']==10001 && ret.toObj()['c']=='abcdefghijkl10001')
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
			println("expected:{id:1,b:10001,c:'abcdefghijkl10001'}");
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
   var db = new Sdb(COORDHOSTNAME, COORDSVCNAME) ;
   db.setSessionAttr( { PreferedInstance: "M" } );
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

