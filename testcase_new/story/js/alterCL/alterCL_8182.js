/******************************************************************************
*@Description : 1. normal collection with no data altered to range-collection
*@Modify list :
*               2014-07-08  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
******************************************************************************/
function main()
{
   var clName = COMMCLNAME + "_8182";
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
   commDropCL( db, COMMCSNAME, clName, true, true,
               "drop colleciton in the beginning" );
}
catch( e )
{
   println( "failed to clean in the beginning" + e ) ;
   throw e ;
}

	//create normal-CL
	try{
      var optionObj = {ReplSize:0};
      var normalCL = commCreateCLByOption( db, COMMCSNAME, clName, optionObj, true,
                                          false, "create collecton failed" );
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+clName});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create normal-CL:" + clName + " rc="+e);
		throw e;
	}
	println("createCL " + clName + " at ReplicaGroup:" + sourceGroup + " finished");
	
	//normalCL with no data altered to range-collection
	try{
		normalCL.alter({ShardingKey:{id:1},ShardingType:'range'});
	}catch(e)
	{
		println("normalCL altered to range-collection fail! rc="+e);
		throw e;
	}
	println("normalCL altered to range-collection finish!");
	
	//split
	try{
		if(groups_num>1){
			var tarGroupIndex=-1;
			var stepId = 1000;
			var partId = 3;
			var lowId = 0;
			var highId = 0;
			for(var i=0;i<partId;i++){
				tarGroupIndex++;
				if(tarGroupIndex == groups_num)
					tarGroupIndex=0;
				if(grouplist[tarGroupIndex]==sourceGroup)
				{
					i--;
					continue;
				}
				lowId = (i-1)*stepId;
				highId = i*stepId;
				normalCL.split(sourceGroup, grouplist[tarGroupIndex],{id:lowId},{id:highId});
				println(clName+" split from "+sourceGroup+" to "+ grouplist[tarGroupIndex]+" {id:"+lowId+"} {id:"+highId+"}");
			}
			println("split succ!");
		}
		else{
			println("can't split to groups!groupsNum is "+groups_num);
		}
	}catch(e)
	{
		println("split fail! rc="+e);
		throw e;
	}
	
	//insert data
	try{
		for(var i=0;i<3000;i++){normalCL.insert({id:i-1000,b:i,c:"abcdefghijkl"+i});}
	}catch(e)
	{
		println("insert-data fail! rc="+e);
		throw e;
	}
	println("insert-data succ!");
	
	//select * from bar where id=1
	//expect one record
	try{
		var sel = normalCL.find({id:{$et:1}});
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
		if(e==-1)
			println("result-records count not expected. expect:1 return:"+size);
		else if(e==-2)
		{	
			println("record not expected!");
			println("expected:{id:1,b:10001,c:'abcdefghijkl1001'}");
			println("returned:"+ret);
		}
		else
			println("select " + clName + " fail! rc="+e);
		throw e;
	}
	println("data-verify succ!");
	
	//clean test-env
	try{
      commDropCL( db, COMMCSNAME, clName, false, false,
                  "drop colleciton in the end" );
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

