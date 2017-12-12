/******************************************************************************
@Description : 1. no element sort
@Modify list :
               2015-01-17 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
rownum = 4;

try{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}catch( e ){
   println( "failed to drop cl, rc = " + e );
   throw e;
}

//create CS
try{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
	var varCL = varCS.createCL(COMMCLNAME,{ReplSize:0});
}catch(e)
{
	println("can't create CS:" + COMMCSNAME + " rc="+e);
	throw e;
}
println("createCS " + COMMCSNAME + " finished");

//insert data
try{
   varCL.insert({a:1,b:"string"});
   varCL.insert({b:"max"});
   varCL.insert({a:{"$binary":"aGVsbG8gd29ybGQ="},b:"min"});
   varCL.insert({b:"empty"});
}catch(e){
	println("insert data failed!");
  throw e;
}
println("insert data finished!");

//query1
//select a,b from foo.bar order by a
try{
	var sel = varCL.find(null,{a:null,b:'b'}).sort({a:1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		i++;
		if(i>rownum){
			break;
		}
	}
	sel.close();
	if(flag && i!=rownum){
		flag = false;
		throw "query1-result-uncorrect";
	}
}catch(e){
	if(e!="query1-result-uncorrect"){
		println("'select a,b from foo.bar order by b' failed! rc="+e);
		throw e;
	}else{
		println("'select a,b from foo.bar order by b' verify record fail!");
  	throw e;
  }
}
println("'select a,b,type from foo.bar order by b' finished!");

//create index
try{
	varCL.createIndex(CLINDEX1,{a:1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query2
//select a,b from foo.bar order by a
try{
	var sel = varCL.find(null,{a:null,b:'b'}).sort({a:1}).hint({"":CLINDEX1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		i++;
		if(i>rownum){
			break;
		}
	}
	sel.close();
	if(flag && i!=rownum){
		flag = false;
		throw "query2-result-uncorrect";
	}
}catch(e){
	if(e!="query2-result-uncorrect"){
		println("'select a,b from foo.bar order by b' with index failed! rc="+e);
		throw e;
	}else{
		println("'select a,b from foo.bar order by b' with index verify record fail!");
  	throw e;
  }
}
println("'select a,b from foo.bar order by b' with index finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
