/******************************************************************************
*@Description : 1. range collection alters replsize
*@Modify list :
*               2014-07-07  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
*               2017-02-08  zhaoyu   Changed
******************************************************************************/
function main()
{
   var clName = COMMCLNAME + "_8177";
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
		var group_num = grouplist.length;
		if(group_num == 1)
		{
			println("only one ReplicaGroup:" + grouplist + " Skip the testcase");
			return;
		}
	}catch(e)
	{
		println("get ReplicaGroups info fail! rc="+e);
		throw e;
	}
	println("ReplicaGroups: " + grouplist);
	
   try
   {
      commDropCL( db, COMMCSNAME, clName, true, true, "drop colleciton" );
   }
   catch( e )
   {
      println( "failed to clean in the beginning" + e ) ;
      throw e ;
   }

	//create range-cl
	try{
      var optionObj = {ShardingKey:{id:1},ShardingType:'range',ReplSize:1};
      var rangeCL = commCreateCLByOption( db, COMMCSNAME, clName, optionObj, true,
                                       false, "create collecton failed" );
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+clName});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create range-CL:" + clName + " rc="+e);
		throw e;
	}
	println("createCL " + clName + " at ReplicaGroup:" + sourceGroup + " finished");
	
	//rangeCL-noSplit alters replsize
	try{
		rangeCL.alter({ReplSize:2});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + clName});
		var replsize = sn1.current().toObj()['ReplSize'];
		if(replsize == 2)
		{
			println("rangeCL-noSplit alters replsize succ!");
		}
		else
		{
			println("rangeCL-noSplit alters replsize fail! ReplSize=" + replsize);
			throw 1;
		}	
	}catch(e)
	{
		if(e != -1)
		{
			println("rangeCL-noSplit alters replsize fail! rc="+e);
		}
		throw e;
	}
	println("rangeCL-noSplit alters replsize finish!");
	
	//split ({id:0} {id:1000})
	try{
		var tarGroupIndex=-1;
		var stepId = 1000;
		var partId = 3;
		var lowId = 0;
		var highId = 0;
		for(var i=0;i<partId;i++){
			tarGroupIndex++;
			if(tarGroupIndex == group_num)
				tarGroupIndex=0;
			if(grouplist[tarGroupIndex]==sourceGroup)
			{
				i--;
				continue;
			}
			lowId = (i-1)*stepId;
			highId = i*stepId;
			rangeCL.split(sourceGroup, grouplist[tarGroupIndex],{id:lowId},{id:highId});
			println(clName+" split from "+sourceGroup+" to "+ grouplist[tarGroupIndex]+" {id:"+lowId+"} {id:"+highId+"}");
		}
	}catch(e)
	{
		println("split rangeCL fail! rc="+e);
		throw e;
	}
	println("split rangeCL succ!");
	
	//rangeCL-split alters replsize
	try{
		rangeCL.alter({ReplSize:3});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + clName});
		var replsize = sn1.current().toObj()['ReplSize'];
		if(replsize == 3)
		{
			println("rangeCL-split alters replsize succ!");
		}
		else
		{
			println("rangeCL-split alters replsize fail! ReplSize=" + replsize);
			throw -1;
		}	
	}catch(e)
	{
		if(e != -1)
		{
			println("rangeCL-split alters replsize fail! rc="+e);
		}
		throw e;
	}
	println("rangeCL-split alters replsize finish!");
	
	//insert data
	try{
		for(var i=0;i<3000;i++){rangeCL.insert({id:i-1000,b:i,c:"abcdefghijkl"+i});}
	}catch(e)
	{
		println("insert-data into rangeCL fail! rc="+e);
	}
	println("insert-data into rangeCL succ!");
	
	//select * from bar where id=1
	//expect one record
	try{
		var sel = rangeCL.find({id:{$et:1}});
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
			throw -1;
		}
		if(!flag)
		{
			throw -2;
		}	
	}catch(e)
	{
		if(e==-1)
			println("result-records count not expected. expect:1 return:"+size);
		else if(e==-2)
		{	
			println("record not expected!");
			println("expected:{id:1,b:1001,c:'abcdefghijkl1001'}");
			println("returned:"+ret);
		}
		else
			println("select " + clName + " fail! rc="+e);
		throw e;
	}
	println("data-verify succ!");
	
	//clean test-env
	try{
      commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton 4" );
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

