/******************************************************************************
@Description : 1. explain verify
@Modify list :
               2014-11-11 pusheng Ding  Init
******************************************************************************/

CHANGEDPREFIX_IDX = CHANGEDPREFIX+"idx" ;

if( false == commIsStandalone( db ) )
{
   // set session get data from master
   db.setSessionAttr( {"PreferedInstance":"M"} ) ;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}
catch(e)
{
   println( "unexpected err happened when clear cs:" + e ) ;
   throw e ;
}

//create CS
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
}
catch ( e )
{
   println("failed to create cs,rc="+ e );
   throw e ;
}

//create CL
try{
	var varCL = varCS.createCL(COMMCLNAME, { ReplSize:0});
}catch(e)
{
	println("can't create normal-CL:" + COMMCLNAME + " rc="+e);
	throw e;
}
println("create " + COMMCLNAME + " finished");

//insert data
try{
   var recs = [];
	for(var i=0;i<100;i++){ 
	   var rec = {a:i-100,b:i,c:"abcdefghijkl"+i};
	   recs.push( rec ); 
   }
	varCL.insert(recs);
}catch(e)
{
	println("insert-data to " + COMMCLNAME + " fail! rc="+e);
	throw e;
}
println("insert data finished");


//find().sort({a:1}).explain()
println("\n***********************************************************");
println(COMMCLNAME + ".find().sort({a:1}).explain() begin...");
try{
	var expl = varCL.find().sort({a:1}).explain().toArray();
	var obj = eval('(' + expl + ')');
	if(obj['ReturnNum'] != 0){
		println("expect:");
		println("\t'ReturnNum':0");
		println("return:");
		println("\t'ReturnNum':" + obj['ReturnNum']);
		throw "explain1-error";
	}
	if(obj['ScanType'] != 'tbscan'){
		println("expect:");
		println("\t'ScanType':'tbscan'");
		println("return:");
		println("\t'ScanType':'" + obj['ScanType'] + "'");
		throw "explain1-error";
	}
	if(obj['UseExtSort'] != true){
		println("expect:");
		println("\t'UseExtSort':true");
		println("return:");
		println("\t'UseExtSort':" + obj['UseExtSort']);
		throw "explain1-error";
	}
	if(obj['Name'] != (COMMCSNAME+'.'+COMMCLNAME)){
		println("expect:");
		println("\t'Name':'" + (COMMCSNAME+'.'+COMMCLNAME) +"'");
		println("return:");
		println("\t'Name':'" + obj['Name'] +"'");
		throw "explain1-error";
	}
	if(obj['IndexName'] != ''){
		println("expect:");
		println("\t'IndexName':''");
		println("return:");
		println("\t'IndexName':'" + obj['IndexName'] +"'");
		throw "explain1-error";
	}
	if(obj['IndexRead'] != 0 || obj['DataRead'] != 0 ){
		println("explain's statistics are error!");
		println("expect:");
		println("\t'IndexRead':0");
		println("\t'DataRead':0");
		println("return:");
		println("\t'IndexRead':" + obj['IndexRead']);
		println("\t'DataRead':" + obj['DataRead']);
		throw "explain1-error";
	}
}catch(e)
{
	if(e!="explain1-error")
	{
		println(COMMCLNAME + ".find().sort({a:1}).explain() fail! rc=" + e + ", expl is:" + expl );
	}
	else
	{
		println("explain is not expected.Returned explain:");
		println(expl);
	}
	
	throw e;
}
println(COMMCLNAME + ".find().sort({a:1}).explain() finished");

//find().sort({a:1}).skip(50).explain({Run:true})
println("\n***********************************************************");
println(COMMCLNAME + ".find().sort({a:1}).skip(50).explain({Run:true}) begin...");
try{
	var expl = varCL.find().sort({a:1}).skip(50).explain({Run:true}).toArray();
	var obj = eval('(' + expl + ')');
	if(obj['ReturnNum'] != 50 && commIsStandalone( db ) ){
		println("standalone mode:");
		println("expect:");
		println("\t'ReturnNum':50");
		println("return:");
		println("\t'ReturnNum':" + obj['ReturnNum']);
		throw "explain2-error";
	}
	// is the collection is only in one node, the skip will down to data node
	if(obj['ReturnNum'] != 50 && false == commIsStandalone( db ) ){
		println("cluster mode:");
		println("expect:");
		println("\t'ReturnNum':50");
		println("return:");
		println("\t'ReturnNum':" + obj['ReturnNum']);
		throw "explain2-error";
	}
	if(obj['ScanType'] != 'tbscan'){
		println("expect:");
		println("\t'ScanType':'tbscan'");
		println("return:");
		println("\t'ScanType':'" + obj['ScanType'] + "'");
		throw "explain2-error";
	}
	if(obj['UseExtSort'] != true){
		println("expect:");
		println("\t'UseExtSort':true");
		println("return:");
		println("\t'UseExtSort':" + obj['UseExtSort']);
		throw "explain2-error";
	}
	if(obj['Name'] != (COMMCSNAME+'.'+COMMCLNAME)){
		println("expect:");
		println("\t'Name':'" + (COMMCSNAME+'.'+COMMCLNAME) +"'");
		println("return:");
		println("\t'Name':'" + obj['Name'] +"'");
		throw "explain2-error";
	}
	if(obj['IndexName'] != ''){
		println("expect:");
		println("\t'IndexName':''");
		println("return:");
		println("\t'IndexName':'" + obj['IndexName'] +"'");
		throw "explain2-error";
	}
	if(obj['IndexRead'] != 0 || obj['DataRead'] == 0){
		println("explain's statistics are error!");
		println("expect:");
		println("\t'IndexRead':0");
		println("\t'DataRead':0");
		println("return:");
		println("\t'IndexRead':" + obj['IndexRead']);
		println("\t'DataRead':" + obj['DataRead']);
		throw "explain2-error";
	}
}catch(e)
{
	if(e!="explain2-error")
	{
		println(COMMCLNAME + ".find().sort({a:1}).skip(50).explain({Run:true}) fail! rc=" + e);
	}
	else
	{
		println("explain is not expected.Returned explain:");
		println(expl);
	}
	
	throw e;
}
println(COMMCLNAME + ".find().sort({a:1}).skip(50).explain({Run:true}) finished");

//create index
try{
	varCL.createIndex(CHANGEDPREFIX_IDX,{a:1});
}catch(e)
{
	println("cat't create index " + CHANGEDPREFIX_IDX + " rc=" + e);
	throw e;
}
println("create index finished!");

//sleep(5);

//find.hint
println("\n***********************************************************");
println(COMMCLNAME + ".find().sort({a:1}).hint(index).explain() begin...");
try{
	var expl = varCL.find().sort({a:1}).hint({"":CHANGEDPREFIX_IDX}).explain().toArray();
	var obj = eval('(' + expl + ')');
	if(obj['ReturnNum'] != 0){
		println("expect:");
		println("\t'ReturnNum':0");
		println("return:");
		println("\t'ReturnNum':" + obj['ReturnNum']);
		throw "explain3-error";
	}
	if(obj['ScanType'] != 'ixscan'){
		println("expect:");
		println("\t'ScanType':'ixscan'");
		println("return:");
		println("\t'ScanType':'" + obj['ScanType'] + "'");
		throw "explain3-error";
	}
	if(obj['UseExtSort'] != false){
		pprintln("expect:");
		println("\t'UseExtSort':false");
		println("return:");
		println("\t'UseExtSort':" + obj['UseExtSort']);
		throw "explain3-error";
	}
	if(obj['Name'] != (COMMCSNAME+'.'+COMMCLNAME)){
		println("expect:");
		println("\t'Name':'" + (COMMCSNAME+'.'+COMMCLNAME) +"'");
		println("return:");
		println("\t'Name':'" + obj['Name'] +"'");
		throw "explain3-error";
	}
	if(obj['IndexName'] != CHANGEDPREFIX_IDX){
		println("expect:");
		println("\t'IndexName':'" + (CHANGEDPREFIX_IDX) +"'");
		println("return:");
		println("\t'IndexName':'" + obj['Name'] +"'");
		throw "explain3-error";
	}
	if(obj['IndexRead'] != 0 || obj['DataRead'] != 0){
		println("explain's statistics are error!");
		throw "explain3-error";
	}
}catch(e)
{
	if(e!="explain3-error")
	{
		println(COMMCLNAME + ".find().sort({a:1}).hint(index).explain({Run:true}) fail! rc=" + e);
	}
	else
	{
		println("explain is not expected.Returned explain:");
		println(expl);
	}
	
	throw e;
}
println(COMMCLNAME + ".find().sort({a:1}).hint(index).explain() finished");

//clean test-env
try{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop cl in the end" ) ;
}catch(e)
{
	println("clean test-evn fail! rc="+e);
	throw e;
}
println("clean test-evn succ!");