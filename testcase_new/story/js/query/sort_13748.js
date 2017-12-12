/******************************************************************************
@Description : 1. binary sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;

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
	//{a:"book",b:2, c:"abcd"}
	varCL.insert({a:{"$binary":"Ym9vaw=="},b:2, c:"abcd"});
	//{a:"agree",b:1, c:"efghi"}
	varCL.insert({a:{"$binary":"YWdyZWU="},b:1, c:"efghi"});
	//{a:"dog",b:4,c:"xyz"}
	varCL.insert({a:{"$binary":"ZG9n"},b:4,c:"xyz"});
	//{a:"cat",b:3, c:"jklmn"}
	varCL.insert({a:{"$binary":"Y2F0"},b:3, c:"jklmn"});
}catch(e)
{
	println("insert-data into varCL fail! rc="+e);
}
println("insert-data into varCL succ!");

//query1
//select a,b from foo.bar order by a
try{
	var sel = varCL.find(null,{a:"default",b:0}).sort({a:1});
	var flag=true;
	//expected result {a:"agree",b:1} {a:"book",b:2} {a:"cat",b:3} {a:"dog",b:4}
	var i = 0;
	var rownum = 4;
	while(sel.next()){
		i++;
		var ret = sel.current();
		println(ret);
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
		println("'select a,b from foo.bar order by a' fail! rc="+e);
		throw e;
	}else{
		println("'select a,b from foo.bar order by a' verify record fail!");
		throw e;
	}
}
println("'select a,b from foo.bar order by a' finished!");

//create index
try{
	varCL.createIndex(CLINDEX1,{a:-1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query2
//select a,b,c from foo.bar order by a desc
try{
	var sel = varCL.find(null,{a:"default",b:0,c:"default"}).sort({a:-1}).hint({"":CLINDEX1});
	var flag=true;
	//expected result {a:"agree",b:1} {a:"book",b:2} {a:"cat",b:3} {a:"dog",b:4}
	var rownum = 4;
	var i = rownum;
	while(sel.next()){
		var ret = sel.current();
		println(ret);
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
		println("'select a,b,c from foo.bar order by a desc' fail! rc="+e);
		throw e;
	}else{
		println("'select a,b,c from foo.bar order by a desc' verify record fail!");
		throw e;
	}
}
println("'select a,b,c from foo.bar order by a desc' finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
