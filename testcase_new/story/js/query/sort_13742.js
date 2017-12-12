/******************************************************************************
@Description : 1. range-cl sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
rownums = 10000;

function main(){
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
	
   try{
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
                  "drop cl in the beginning" ) ;
   }catch( e ){
      println( "failed to drop cl, rc = " + e );
      throw e;
   }

	//create CS
	try{
      var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
	}catch(e)
	{
		println("can't create CS:" + COMMCSNAME + " rc="+e);
		throw e;
	}
	println("createCS " + COMMCSNAME + " finished");
	
	//create range-cl
	try{
		var rangeCL = varCS.createCL(COMMCLNAME,{ShardingKey:{a:1},ShardingType:'range',ReplSize:0});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+COMMCLNAME});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create range-CL:" + COMMCLNAME + " rc="+e);
		throw e;
	}
	println("createCL " + COMMCLNAME + " at ReplicaGroup:" + sourceGroup + " finished");
	
	//split ({a:0} {a:1000})
	try{
		var tarGroupIndex=-1;
		var stepId = 1000;
		var partId = rownums/stepId;
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
			rangeCL.split(sourceGroup, grouplist[tarGroupIndex],{a:lowId},{a:highId});
			println(COMMCLNAME+" split from "+sourceGroup+" to "+ grouplist[tarGroupIndex]+" {a:"+lowId+"} {a:"+highId+"}");
		}
	}catch(e)
	{
		println("split rangeCL fail! rc="+e);
		throw e;
	}
	println("split rangeCL succ!");
	
	//insert data
	try{
		for(var i=0;i<rownums;i++){rangeCL.insert({a:rownums-i,b:i,c:"abcdefghijkl"+i});}
	}catch(e)
	{
		println("insert-data into rangeCL fail! rc="+e);
	}
	println("insert-data into rangeCL succ!");
	
	//query1
	//select a,b,c from foo.bar order by a desc
	try{
		var sel = rangeCL.find(null,{a:0,b:'b',c:'c'}).sort({a:-1});
		var flag=true;
		//expected result {a:rownums,...} {a:rownums-1,...} ... {a:1,...}
		var i = rownums;
		while(sel.next()){
			var ret = sel.current();
			if(ret.toObj()['a']!=i){
				flag = false;
				throw "query1-result-uncorrect";
			}
			i--;
			if(i<0){
				break;
			}
		}
		sel.close();
		if(flag && i!=0){
			flag = false;
			throw "query1-result-uncorrect";
		}
	}catch(e){
		if(e!="query1-result-uncorrect"){
			println("'select a,b,c from foo.bar order by a desc' fail! rc="+e);
			throw e;
		}else{
			println("'select a,b,c from foo.bar order by a desc' verify record fail!");
			throw e;
		}
	}
	println("'select a,b,c from foo.bar order by a desc' finished!");

	//create index
	try{
		rangeCL.createIndex(CLINDEX1,{b:1});
	}catch( e ){
	   println("create indexes fail");
	   throw e ;	
	}
	println("create indexes finished!");
	
	//query2
	//select b from foo.bar order by b
	try{
		var sel = rangeCL.find(null,{b:'b'}).sort({b:1}).hint({"":CLINDEX1});
		var flag=true;
		//expected result {b:0} {b:1} ... {b:rownums-1}
		var i = 0;
		while(sel.next()){
			var ret = sel.current();
			if(ret.toObj()['b']!=i){
				flag = false;
				throw "query2-result-uncorrect";
			}
			i++;
			if(i>rownums){
				break;
			}
		}
		sel.close();
		if(flag && i!=rownums){
			flag = false;
			throw "query2-result-uncorrect";
		}
	}catch(e){
		if(e!="query2-result-uncorrect"){
			println("'select b from foo.bar order by b' fail! rc="+e);
			throw e;
		}else{
			println("'select b from foo.bar order by b' verify record fail!");
			throw e;
		}
	}
	println("'select b from foo.bar order by b' finished!");
	
	try
	{
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
	}
	catch ( e )
	{
	   println( "failed to drop cs, rc= " + e ) ;
	   throw e ;
	}
	
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
