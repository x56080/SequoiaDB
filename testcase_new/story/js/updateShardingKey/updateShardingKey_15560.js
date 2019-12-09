/************************************
*@Description: shardedCL update ShardingKey,
                  the ShardingKey are updated fail
*@author:      wuyan
*@createdate:  2018.7.29
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_15560";
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var shardingKey = { no: 1 };
   var dbcl = createCL( COMMCSNAME, clName, shardingKey );

   //insert data 	
   var doc = [{ no: "testupdate", a: "testa3", b: 3 }];
   insertData( dbcl, doc );

   //update ShardingKey,set KeepShardingKey=true
   var updateCondition = { $set: { no: "testupdate", a: "testa" } };
   try
   {
      println( "---begin to update shardingKey" );
      dbcl.update( updateCondition, {}, {}, { KeepShardingKey: true } );
      throw "updateErrExcute fail!";
   }
   catch( e )
   {
      if( -178 != e )
      {
         throw buildException( "updateDataError()", e );
      }
   }

   var findCondition = { b: { $gte: 2 } };
   var upsertCondition = { $set: { no: "testupdate12807", a: "testa12807" } };
   try
   {
      println( "---begin to upsert shardingKey" );
      dbcl.upsert( upsertCondition, findCondition, {}, {}, { KeepShardingKey: true } );
      throw "upsertExcutefail!";
   }
   catch( e )
   {
      if( -178 != e )
      {
         throw buildException( "upsertDataError()", e );
      }
   }

   //check the update result	
   checkResult( dbcl, null, null, doc, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}
main();
