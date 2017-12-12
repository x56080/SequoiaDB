/******************************************************************************
@Description : 1. sort: index sort
@Modify list :
               2015-01-15 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
CLINDEX2 = CHANGEDPREFIX + "IND2" ;
rownums = 10000;

try{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}catch( e ){
   println( "failed to drop cl, rc = " + e );
   throw e;
}

//create index
try{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME );
	varCL.createIndex(CLINDEX1,{a:1});
	varCL.createIndex(CLINDEX2,{b:-1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//insert data
try{
   for(var i=0; i<rownums; i++){
   	varCL.insert({a:i,b:i,c:i+"abcdefghijklmnopqrstuvwxyz"});
  }
}catch(e){
	println("insert data failed!");
  throw e;
}
println("insert data finished!");

//query1
//select a,b,c from foo.bar order by a
try{
	var sel = varCL.find(null,{a:null,b:'b',c:'c'}).sort({a:1}).hint({"":CLINDEX1});
	var flag=true;
	//expected result {a:0,...} {a:1,...} ... {a:9999,...}
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		if(ret.toObj()['a']!=i){
			flag = false;
			throw "query1-result-uncorrect";
		}
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
		println("'select a,b,c from foo.bar order by a' with index failed! rc="+e);
		throw e;
	}else{
		println("'select a,b,c from foo.bar order by a' with index verify record fail!");
  	throw e;
  }
}
println("'select a,b,c from foo.bar order by a' with index finished!");

//query2
//select b,a from foo.bar order by b desc
try{
	var sel = varCL.find(null,{b:'b',a:null}).sort({b:-1}).hint({"":CLINDEX2});
	var flag=true;
	//expected result {a:9999,...} {a:9998,...} ... {a:0,...}
	var i = rownums;
	while(sel.next()){
		var ret = sel.current();
		if(ret.toObj()['a']!=(i-1)){
			flag = false;
			throw "query2-result-uncorrect";
		}
		i--;
		if(i<0){
			break;
		}
	}
	sel.close();
	if(flag && i!=0){
		flag = false;
		throw "query2-result-uncorrect";
	}
}catch(e){
	if(e!="query2-result-uncorrect"){
		println("'select b,a from foo.bar order by b desc' with index failed! rc="+e);
		throw e;
	}else{
		println("'select b,a from foo.bar order by b desc' with index verify record fail!");
  	throw e;
  }
}
println("'select b,a from foo.bar order by b desc' with index finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
