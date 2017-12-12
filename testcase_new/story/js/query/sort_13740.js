/******************************************************************************
@Description : 1.columnts-number to sort: 16 columns
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
		varCL.insert({a1:i,a2:i+1,a3:i+2,a4:i+3,a5:i+4,a6:i+5,a7:i+6,a8:i+7,a9:i+8,a10:i+9,a11:i+10,a12:i+11,a13:i+12,a14:i+13,a15:i+14,a16:i+15});
	}
}catch( e ){
	println("insert data failed!");
  throw e;
}
println("insert data finished!");

//query1
//select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16
try{
	var sel = varCL.find(null,{a1:null,a2:null,a3:null,a4:null,a5:null,a6:null,a7:null,a8:null,a9:null,a10:null,a11:null,a12:null,a13:null,a14:null,a15:null,a16:null}).sort({a1:1,a2:1,a3:1,a4:1,a5:1,a6:1,a7:1,a8:1,a9:1,a10:1,a11:1,a12:1,a13:1,a14:1,a15:1,a16:1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		if(ret.toObj()['a1']!=i){
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
		println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' failed! rc="+e);
		throw e;
	}else{
		println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' verify record fail!");
  	throw e;
  }
}
println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' finished!");

//create index
try{
	varCL.createIndex(CLINDEX1,{b:1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query2
//select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16
try{
	var sel = varCL.find(null,{a1:null,a2:null,a3:null,a4:null,a5:null,a6:null,a7:null,a8:null,a9:null,a10:null,a11:null,a12:null,a13:null,a14:null,a15:null,a16:null}).sort({a1:1,a2:1,a3:1,a4:1,a5:1,a6:1,a7:1,a8:1,a9:1,a10:1,a11:1,a12:1,a13:1,a14:1,a15:1,a16:1}).hint({"":CLINDEX1});
	var flag=true;
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		if(ret.toObj()['a1']!=i){
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
		println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' with index failed! rc="+e);
		throw e;
	}else{
		println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' with index verify record fail!");
  	throw e;
  }
}
println("'select a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16 from foo.bar order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16' with index finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
