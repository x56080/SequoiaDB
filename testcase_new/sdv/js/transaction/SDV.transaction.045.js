/************************************************************************
*@Description:	seqDB-6071：主子表中插入数据不在分区范围内_SD.transaction.045
               分别执行开启事务、删除、插入不在子表分区范围内的记录、提交
*@Author:  		TingYU  2015/11/25
               wuyan 2017/1/6(修改重复执行回滚不报错) 
************************************************************************/
/*main();

function main()
{
   var csName = COMMCSNAME;
   var mainclName = COMMCLNAME + "_maincl";
   var subclName  = COMMCLNAME + "_subcl";   
   
   try
   {
      if( !commIsTransEnabled( db ) )
      {
         println( "transaction is disabled" );
         return;
      }
      
      var maincl = readyCL( csName, mainclName, subclName );
      
      //begin transaction and remove      
      var dataNum = 100;
      var insert = new insertData( maincl, dataNum );
      var remove = new removeData( maincl );
      execTransaction( insert, beginTrans, remove );
      checkResult( maincl, true, remove );
      
      //insert a record that is out of range
      try
      {   
         maincl.insert({mainSk:101});
         throw buildException( "insert", "", " maincl.insert({mainSK:101})",
                               -135, "did not throw any error" );
      }
      catch(e)
      {
         var expErr = -135;
         if( e !== expErr )
         {
            throw e;
         }
      }
      checkResult( maincl, false, remove );
      
      //commit
      try
      {   
         execTransaction( commitTrans );
        // throw buildException( "commitTrans()", "", "excute commit after insert fail",
                              // -196, "did not throw any error" );
      }
      catch(e)
      {
        // var expErr = "commitTrans() unknown error expect: " + -196;
        // if( e !== expErr )
         //{
            throw e;
         //}
      }
      checkResult( maincl, false, remove );
                    
	   clean( csName, mainclName );
	   clean( csName, subclName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
   }            
}

function readyCL( csName, mainclName, subclName )
{
	println( "--create maincl subcl" );
	
	commDropCL( db, csName, mainclName, true, true, "drop main cl in begin" );	
	commDropCL( db, csName, subclName , true, true, "drop sub cl in begin" );	
	
	var mainOpt = {ShardingKey:{mainSk:1}, ShardingType:"range", IsMainCL:true, ReplSize:0 };
	var subOpt  = {ReplSize:0};
   var maincl = 
   commCreateCLByOption( db, csName, mainclName, mainOpt, true, false, "create mian cl in begin" ); 
   commCreateCLByOption( db, csName, subclName , subOpt , true, false, "create sub cl in begin" );
   
   println( "--attach cl" );
   
   var attaOpt = { LowBound: {mainSk:{$minKey:1}}, UpBound: {mainSk:100} }; 
   maincl.attachCL( csName+"."+subclName, attaOpt );
   
   return maincl;
}*/