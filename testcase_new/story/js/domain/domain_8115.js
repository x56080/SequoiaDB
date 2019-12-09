/******************************************************************************
@Description : 1. Test before create domain range/percent split and AutoSplit.
               2. Test insert/update/find/remove operation.
@Modify list :
               2014-6-25  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomRangeHashAutoSplit";
   var rangeCL = clName + "_Range";
   var commonCL = clName + "_Common";

   // Drop CS
   commDropCS( db, csName, clName, true,
      "clear collection space in the beginning" );

   // Drop domain
   clearDomain( db, domName );


   /***************************************************************************
   @ Begin to create domain and do AutoSplit operation
   ***************************************************************************/
   // Get all data groups and create domain by specify AutoSplit
   try
   {
      println( "Begin to create domain" );
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      createDomain( db, domName, group, { "AutoSplit": true } );
   }
   catch( e )
   {
      if( -159 != e )
      {
         println( "Failed to create domain, rc = " + e );
         throw e;
      }
      else
         println( "The mode is standalong, not group" );
   }

   // Inspect domain
   inspectDomain( db, domName );

   // Create CS in domain and create collection
   try
   {
      commCreateCS( db, csName, false, "create CS specify domain",
         { "Domain": domName } );
      commCreateCLByOption( db, csName, rangeCL, {
         ShardingKey: { "No": -1 },
         ShardingType: "range", Partition: 1024,
         ReplSize: 0
      },
         false, false, "create collection in domain" );
      // Comman CL
      commCreateCLByOption( db, csName, commonCL, {
         ShardingKey: { "No": -1 },
         ReplSize: 0
      },
         false, false, "create collection in domain" );
   }
   catch( e )
   {
      println( "Failed to create CS by specify domain, rc = " + e );
      throw e;
   }

   // Insert data
   insertData( db, csName, rangeCL, 1000 );
   insertData( db, csName, commonCL, 1000 );

   // inspect the AutoSplit is take effect or not
   inspectAutoSplit( db, csName, rangeCL, domName );
   inspectAutoSplit( db, csName, commonCL, domName );
   println( "Success to inspect the autosplit parameter" );

   // Query data
   queryData( db, csName, rangeCL );
   queryData( db, csName, commonCL );

   // Update data
   updateData( db, csName, rangeCL );
   updateData( db, csName, commonCL );

   // Remove data
   removeData( db, csName, rangeCL );
   removeData( db, csName, commonCL );

   // Drop domain in the end
   clearDomain( db, domName );
   println( "Success to clean domain : [" + domName + "] in the end" );


}

try
{
   main( db );
   db.close();
}
catch( e )
{
   if( "RunMode_StandAlone" != e )
      throw e;
   else
      println( "WARNNING! Run Mode is : [ standalone ]" );

}
