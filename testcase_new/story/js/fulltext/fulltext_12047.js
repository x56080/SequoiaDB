/***************************************************************************
@Description :seqDB-12047 :hash分区表中执行全文检索  
@Modify list :2018-11-22  Zhaoxiaoni  init
****************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      println( "Deploy one group" );
      return;
   }

   var clName = COMMCLNAME + "_ES_12047";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_12047", { a: "text", b: "text" } );

   var records = [];
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "i" };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12047", 10000 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { Partion: 1 }, { Partion: 2048 } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12047", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   println( "===find from more group success===" );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12047" );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;
