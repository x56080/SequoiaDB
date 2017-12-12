/******************************************************************************
@Description : 1. sub-CL sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
SUBCL1NAME = CHANGEDPREFIX + "_sub1";
SUBCL2NAME = CHANGEDPREFIX + "_sub2";
SUBCL3NAME = CHANGEDPREFIX + "_sub3";
CLINDEX1 = CHANGEDPREFIX + "IND1" ;
rownums = 10000;

function test_subCl_index_query() 
{

   try{
      commDropCL( db, COMMCSNAME, SUBCL1NAME, true, true,
                  "drop sub cl1 in the beginning" ) ;
      commDropCL( db, COMMCSNAME, SUBCL2NAME, true, true,
                  "drop sub cl2 in the beginning" ) ;
      commDropCL( db, COMMCSNAME, SUBCL3NAME, true, true,
                  "drop sub cl3 in the beginning" ) ;
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
                  "drop main cl in the beginning" ) ;
   }catch( e ){
      println( "failed to drop cl, rc = " + e );
      throw e;
   }

   //create CS
   try{
      var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   }catch(e)
   {
      println("can't create CS:" + COMMCSNAME + " rc="+e);
      throw e;
   }
   println("createCS " + COMMCSNAME + " finished");

   //create main-cl
   try{
      var mainCL = varCS.createCL(COMMCLNAME,{ShardingKey:{a:1} , ReplSize:0,IsMainCL:true});
   }catch(e)
   {
      println("can't create main-CL:" + COMMCLNAME + " rc="+e);
      throw e;
   }
   println("createCL " + COMMCLNAME + " finished");

   //create sub-cl
   try{
      var subCL1 = varCS.createCL(SUBCL1NAME,{ShardingKey:{b:1},ShardingType:"hash",Partition:4096});
      var subCL2 = varCS.createCL(SUBCL2NAME,{ReplSize:2});
      var subCL3 = varCS.createCL(SUBCL3NAME,{Compressed:true});
   }catch(e)
   {
      println("can't create sub-CL");
      throw e;
   }

   //attach sub-cl
   try{
      mainCL.attachCL(COMMCSNAME + "." + SUBCL1NAME,{LowBound:{a:-10000},UpBound:{a:0}});
      mainCL.attachCL(COMMCSNAME + "." + SUBCL2NAME,{LowBound:{a:0},UpBound:{a:6000}});
      mainCL.attachCL(COMMCSNAME + "." + SUBCL3NAME,{LowBound:{a:6000},UpBound:{a:20000}});
   }catch(e)
   {
      if( false == commIsStandalone( db ) )
      {
         println("attach sub-CL fail!");
         throw e;
      }
   }
   println("attach sub-CL finish!");

   //insert data
   try{
      for(var i=0;i<rownums;i++){mainCL.insert({a:rownums-i,b:i,c:"abcdefghijkl"+i});}
   }catch(e)
   {
      println("insert-data into mainCL fail! rc="+e);
   }
   println("insert-data into mainCL succ!");

   //query1
   //select a,b,c from foo.bar order by a desc
   try{
      var sel = mainCL.find(null,{a:0,b:0,c:'c'}).sort({a:-1});
      var flag=true;
      //expected result {a:rownums,...} {a:rownums-1,...} ... {a:1,...}
      var i = rownums;
      while(sel.next()){
         var ret = sel.current();
         if(ret.toObj()['a']!=i){
            flag = false;
            throw "query1-result-uncorrect";
         }
         i--;
         if(i<0){
            break;
         }
      }
      sel.close();
      if(flag && i!=0){
         flag = false;
         throw "query1-result-uncorrect";
      }
   }catch(e){
      if(e!="query1-result-uncorrect"){
         println("'select a,b,c from foo.bar order by a desc' fail! rc="+e);
         throw e;
      }else{
         println("'select a,b,c from foo.bar order by a desc' verify record fail!");
         throw e;
      }
   }
   println("'select a,b,c from foo.bar order by a desc' finished!");

   //create index
   try{
      mainCL.createIndex(CLINDEX1,{a:1, b:1},true);
   }catch( e ){
      println("create indexes fail");
      throw e ;
   }
   println("create indexes finished!");

   //query2
   //select b from foo.bar order by b
   try{
      db.setSessionAttr( {PreferedInstance:'M'} ) ;
      // 走索引查询
      var selExplain = mainCL.find(null,{b:0}).sort({b:1}).hint({"":CLINDEX1}).explain().toArray();
      for( var j = 0; j < selExplain.length; ++j )
      {
         var selObj = eval( "(" + selExplain[i] + ")" );
         if( "ixscan" != selObj["SubCollections"][0]["ScanType"] )
         {
            println( "explain: " + selExplain[i] );
            throw "failed to run index query";
         }
      }
      var sel = mainCL.find(null,{b:0}).sort({b:1}).hint({"":CLINDEX1});
      var flag=true;
      //expected result {b:0} {b:1} ... {b:rownums-1}
      var i = 0;
      while(sel.next()){
         var ret = sel.current();
         if(ret.toObj()['b']!=i){
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
         println("'select b from foo.bar order by b' fail! rc="+e + ", selExplain is: " + selExplain );
         throw e;
      }else{
         println("'select b from foo.bar order by b' verify record fail!");
         throw e;
      }
   }
   println("'select b from foo.bar order by b' finished!");

   //attach sub-cl
   try{
       mainCL.detachCL(COMMCSNAME + "." + SUBCL1NAME);
       mainCL.detachCL(COMMCSNAME + "." + SUBCL2NAME);
       mainCL.detachCL(COMMCSNAME + "." + SUBCL3NAME);
   }catch(e)
   {
      if( false == commIsStandalone( db ) )
      {
         println("detach sub-CL fail!");
         throw e;
      }
   }
   println("detach sub-CL finish!");

   try
   {
       commDropCL( db, COMMCSNAME, SUBCL1NAME, false, false,
                   "drop sub cl1 in the end" ) ;
       commDropCL( db, COMMCSNAME, SUBCL2NAME, false, false,
                   "drop sub cl2 in the end" ) ;
       commDropCL( db, COMMCSNAME, SUBCL3NAME, false, false,
                   "drop sub cl3 in the end" ) ;
       commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
                   "drop main cl in the end" ) ;
   }
   catch ( e )
   {
      println( "failed to drop cs, rc= " + e ) ;
      throw e ;
   }
}

// Add inspect standalone run mode
try
{
   // Inspect the run mode is standalone or not
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone" ;
   test_subCl_index_query();
}
catch( e )
{
   if( "ModeStandAlone" == e )
      println( "The run mode is standalone" ) ;
   else
      throw e ;
}
