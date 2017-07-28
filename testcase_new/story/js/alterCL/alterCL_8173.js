/******************************************************************************
@Description : 1.altered shard basic
@Modify list :
               2015-01-22  pusheng Ding  Init
******************************************************************************/
clName_1 = CHANGEDPREFIX+"_8173_bar1" ;
clName_2 = CHANGEDPREFIX+"_8173_bar2" ;

function main()
{
	try{
		var db = new Sdb(COORDHOSTNAME, COORDSVCNAME) ;
	}catch(e)
	{
		println("can't connect to db");
		throw e;
	}
	
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
	
	try{
      commDropCL( db, COMMCSNAME, clName_1, true, true,
                  "drop cl1 in the beginning" ) ;
      commDropCL( db, COMMCSNAME, clName_2, true, true,
                  "drop cl2 in the beginning" ) ;
	}catch( e ){}
	
	//create CS
	try{
      var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
	}catch(e)
	{
		println("can't create CS:" + COMMCSNAME + " rc="+e);
		throw e;
	}
	println("createCS " + COMMCSNAME + " finished");
	
	//create normal-CL
	try{
		var rangeCL = varCS.createCL(clName_1,{ReplSize:0});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+clName_1});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create normal-CL:" + clName_1 + " rc="+e);
		throw e;
	}
	println("createCL " + clName_1 + " at ReplicaGroup:" + sourceGroup + " finished");
	
	//normalCL with no data altered to range-collection
	try{
		rangeCL.alter({ShardingKey:{id:1},ShardingType:'range'});
	}catch(e)
	{
		println("normalCL altered to range-collection fail! rc="+e);
		throw e;
	}
	println("normalCL altered to range-collection finish!");
	
	//create normal-CL
	try{
		var hashCL = varCS.createCL(clName_2,{ReplSize:0});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+clName_2});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create normal-CL:" + clName_2 + " rc="+e);
		throw e;
	}
	println("createCL " + clName_2 + " at ReplicaGroup:" + sourceGroup + " finished");
	
	//normalCL with no data altered to hash-collection
	try{
		hashCL.alter({ShardingKey:{id:1},ShardingType:'hash', Partition:1024});
	}catch(e)
	{
		println("normalCL altered to hash-collection fail! rc="+e);
		throw e;
	}
	println("normalCL altered to hash-collection finish!");
	
	//clean test-env
	try{
      commDropCL( db, COMMCSNAME, clName_1, true, true,
                  "drop cl1 in the end" ) ;
      commDropCL( db, COMMCSNAME, clName_2, true, true,
                  "drop cl2 in the end" ) ;
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
