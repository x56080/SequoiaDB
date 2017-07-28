/******************************************************************************
*@Description : 1. normal collection alters replsize
*@Modify list :
*               2014-07-08  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
******************************************************************************/
clName = COMMCLNAME + "_8181";

try{
   db.setSessionAttr( { PreferedInstance: "M" } );
   
   commDropCL( db, COMMCSNAME, clName, true, true,
               "drop colleciton in the beginning" );
	var isStandalone = commIsStandalone( db ) ;
}catch(e)
{
	println("can't connect to db");
	throw e;
}

//create normal-CL
try{
   var optionObj = {Compressed:true};
   var normalCL = commCreateCLByOption( db, COMMCSNAME, clName, optionObj, true,
                                       false, "create collecton failed" );
}catch(e)
{
	println("can't create normal-CL:" + clName + " rc="+e);
	throw e;
}
println("createCL " + clName + " finished");

//normalCL alters replsize once
try{
	normalCL.alter({ReplSize:2});
	var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + clName});
	var replsize = sn1.current().toObj()['ReplSize'];
	if(replsize == 2)
	{
		println("normalCL alters replsize succ once!");
	}
	else
	{
		println("normalCL alters replsize fail once! ReplSize=" + replsize);
		throw 1;
	}	
}catch(e)
{
	if(!isStandalone)
	{
		if(e != -1)
		{
			println("normalCL alters replsize fail once! rc="+e);
		}
		throw e;
	}
	else
	{
		println("normalCL alters replsize fail once when standalone");
	}
}
println("normalCL alters replsize finish once!");

//insert data
try{
	for(var i=0;i<10000;i++){normalCL.insert({id:i-10000,b:i,c:"abcdefghijkl"+i});}
}catch(e)
{
	println("insert-data into normalCL fail! rc="+e);
}
println("insert-data into normalCL succ!");

//normalCL alters replsize twice
try{
	normalCL.alter({ReplSize:3});
	var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + clName});
	var replsize = sn1.current().toObj()['ReplSize'];
	if(replsize == 3)
	{
		println("normalCL alters replsize succ twice!");
	}
	else
	{
		println("normalCL alters replsize fail twice! ReplSize=" + replsize);
		throw -1;
	}	
}catch(e)
{
	if(!isStandalone)
	{
		if(e != -1)
		{
			println("normalCL alters replsize fail twice! rc="+e);
		}
		throw e;
	}
	else
	{
		println("normalCL alters replsize fail twice when standalone");
	}
}
println("normalCL alters replsize finish twice!");

//remove data
try{
	normalCL.remove();
}catch(e)
{
	println("remove-data from normalCL fail! rc="+e);
}
println("remove-data from normalCL succ!");

//insert data
try{
	for(var i=0;i<5000;i++){normalCL.insert({id:i,b:i-2500,c:"abcdefghijkl"+i});}
}catch(e)
{
	println("insert-data into normalCL fail! rc="+e);
}
println("insert-data into normalCL succ!");

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
		if(ret.toObj()['id']==1 && ret.toObj()['b']==-2499 && ret.toObj()['c']=='abcdefghijkl1')
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
		println("expected:{id:1,b:10001,c:'abcdefghijkl10001'}");
		println("returned:"+ret);
	}
	else
		println("select " + clName + " fail! rc="+e);
	throw e;
}
println("data-verify succ!");

//clean test-env
try{
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton" );
}catch(e)
{
	println("clean test-evn fail! rc="+e);
	throw e;
}
println("clean test-evn succ!");
