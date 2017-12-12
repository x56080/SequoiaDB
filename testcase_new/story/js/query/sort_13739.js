/******************************************************************************
@Description : 1. sort: a[1,2,3] sort
@Modify list :
               2015-01-15 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
rownums = 10000;

try{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}catch( e ){
   println( "failed to drop cl, rc = " + e );
   throw e;
}

try{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
	var varCL = varCS.createCL(COMMCLNAME,{ReplSize:0});
}catch( e ){
   println("createCS or createCL fail");
   throw e ;	
}

//insert data
try{
	for(var i=0; i<rownums; i++){
		varCL.insert({a:{a1:i,a2:rownums-i},b:[i,i+1,i+2]});
	}
}catch( e ){
	println("insert data failed!");
  throw e;
}
println("insert data finished!");

//query1
//select a,b from foo.bar order by b
try{
	var sel = varCL.find(null,{a:null,b:'b'}).sort({b:1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		i++;
		if(i>rownums){
			break;
		}
	}
	sel.close();
	if(flag && i!=rownums){
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
println("'select a,b from foo.bar order by b' finished!");

//create index
try{
	varCL.createIndex(CLINDEX1,{b:1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query2
//select a,b from foo.bar order by b
try{
	var sel = varCL.find(null,{a:null,b:'b'}).sort({b:1}).hint({"":CLINDEX1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
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
