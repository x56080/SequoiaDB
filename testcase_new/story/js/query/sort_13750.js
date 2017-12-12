/******************************************************************************
@Description : 1. different datatypes sort
@Modify list :
               2015-01-17 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
rownum = 13;

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
   //int
   varCL.insert({a:1,b:100,type:"int"});
   //bigint
   varCL.insert({a:2,b:123456789012,type:"bigint"});
   //float
   varCL.insert({a:3,b:1.1234e-12,type:"float"});
   //string
   varCL.insert({a:4,b:'1234abcd',type:"string"});
   //oid
   varCL.insert({a:5,b:{"$oid":"123abcd00ef12358902300ef"},type:"oid"});
   //boolean
   varCL.insert({a:6,b:true,type:"boolean"});
   //date
   varCL.insert({a:7,b:{"$date":"2015-01-17"},type:"date"});
   //timestamp
   varCL.insert({a:8,b:{"$timestamp":"2015-01-17-10.59.30.124233"},type:"timestamp"});
   //regex
   varCL.insert({a:9,b:{"$regex":"^张","$options":"1"},type:"regex"});
   //object
   varCL.insert({a:10,b:{"subobj":"value"},type:"object"});
   //array
   varCL.insert({a:11,b:["abc",100,"def"],type:"array"});
   //null
   varCL.insert({a:12,b:null,type:"null"});
   //empty
   varCL.insert({a:13,type:"empty"});
}catch(e){
	println("insert data failed!");
  throw e;
}
println("insert data finished!");

//query1
//select a,b,type from foo.bar order by b
try{
	var sel = varCL.find(null,{a:null,b:'b',type:'unknown'}).sort({b:1});
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
		println("'select a,b,type from foo.bar order by b' failed! rc="+e);
		throw e;
	}else{
		println("'select a,b,type from foo.bar order by b' verify record fail!");
  	throw e;
  }
}
println("'select a,b,type from foo.bar order by b' finished!");

//create index
try{
	varCL.createIndex(CLINDEX1,{b:1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query2
//select a,b,type from foo.bar order by b
try{
	var sel = varCL.find(null,{a:null,b:'b',type:'unknown'}).sort({b:1}).hint({"":CLINDEX1});
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
		println("'select a,b,type from foo.bar order by b' with index failed! rc="+e);
		throw e;
	}else{
		println("'select a,b,type from foo.bar order by b' with index verify record fail!");
  	throw e;
  }
}
println("'select a,b,type from foo.bar order by b' with index finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
