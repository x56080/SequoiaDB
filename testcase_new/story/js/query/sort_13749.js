/******************************************************************************
@Description : 1. _id sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
rownums = 1000;

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
try
{
   var recs = [];
   for(var i=0; i<rownums; i++)
   { 
      var rec = {a:i,b:"abcdefghijklmnopq"+i,c:i+"abcdefghijklmnopqrstuvwxyz"};
      recs.push( rec ); 
   }
   varCL.insert(recs);
}
catch(e)
{
   println("insert data failed!");
   throw e;
}
println("insert data finished!");

//query1
//select a,b,c from foo.bar order by _id
try{
	var sel = varCL.find(null,{a:null,b:'b',c:'c'}).sort({"_id":1});
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
		println("'select a,b,c from foo.bar order by _id' failed! rc="+e);
		throw e;
	}else{
		println("'select a,b,c from foo.bar order by _id' verify record fail!");
  	throw e;
  }
}
println("'select a,b,c from foo.bar order by _id' finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
