/* *****************************************************************************
@discretion: query basic: hint
@modify list:
             2015-01-31  Pusheng Ding  Init
***************************************************************************** */
CLINDEX1 = CHANGEDPREFIX + "IND1" ;

rownums = 100;

try{
	var db = new Sdb(COORDHOSTNAME, COORDSVCNAME) ;
}catch(e)
{
	println("can't connect to db");
	throw e;
}

try{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop cl in the beginning" ) ;
}catch( e ){}

//create CS
try{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
}catch(e)
{
	println("can't create CS:" + COMMCSNAME + " rc="+e);
	throw e;
}
println("createCS " + COMMCSNAME + " finished");

//create hash-cl
try{
	var hashCL = varCS.createCL(COMMCLNAME,{ShardingKey:{a:1},ShardingType:'hash',ReplSize:0});
}catch(e)
{
	throw e;
}
println("createCL " + COMMCLNAME + " finished");

//insert data
try{
	for(var i=0;i<rownums;i++){hashCL.insert({a:rownums-i,b:i,c:"abcdefghijkl"+i});}
}catch(e)
{
	println("insert-data into hashCL fail! rc="+e);
}
println("insert-data into hashCL succ!");

//create index
try{
	hashCL.createIndex(CLINDEX1,{b:1});
}catch( e ){
   println("create indexes fail");
   throw e ;	
}
println("create indexes finished!");

//query1
//select b from foo.bar order by b
try{
	var sel = hashCL.find(null,{b:'b'}).sort({b:1}).hint({"":CLINDEX1});
	var flag=true;
	//expected result {b:0} {b:1} ... {b:rownums-1}
	var i = 0;
	while(sel.next()){
		var ret = sel.current();
		if(ret.toObj()['b']!=i){
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
		println("'select b from foo.bar order by b' fail! rc="+e);
		throw e;
	}else{
		println("'select b from foo.bar order by b' verify record fail!");
		println(sel);
		throw e;
	}
}
println("'select b from foo.bar order by b' finished!");

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
