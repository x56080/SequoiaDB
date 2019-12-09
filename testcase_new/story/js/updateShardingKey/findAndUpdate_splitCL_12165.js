/************************************
*@Description: splitCL find and update ShardingKey,set the KeepShardingKey is true
*@author:      wuyan
*@createdate:  2017.7.29
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_12165";
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var shardingKey = { no: 1 };
   var dbcl = createCL( COMMCSNAME, clName, shardingKey );

   //insert data 	
   var doc = [{ no: { "$timestamp": "2017-07-29-13.14.26.124233" }, a: "testa1", b: 1 },
   { no: "testupdate", a: "testa3", b: 3 }];
   insertData( dbcl, doc );

   //split cl                 
   var percent = 50;
   clSplit( COMMCSNAME, clName, percent )

   //update ShardingKey,set KeepShardingKey=true
   var updateCondition = { $set: { no: "testupdate", a: "testa" } };
   updateDataError( dbcl, "findAndUpdate", updateCondition );

   //check the update result
   var expRecs = doc;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}

