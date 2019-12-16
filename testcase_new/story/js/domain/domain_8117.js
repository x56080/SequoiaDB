/******************************************************************************
@Description : Test create Domain specify group : "SYSCatalogGroup".
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domNameSYS = csName + "_DomGroupSYSCata";
   var domName = csName + "_DomAllGroupComprareSYS";

   // Drop domain
   clearDomain( db, domName );

   // Drop CS
   commDropCS( db, csName, clName, true,
      "clear collection space in the beginning" );

   try
   {
      var group = new Array();
      group = getGroup( db );
      // create domain specify {Group : [ "SYSCatalogGroup" ]} [Testing Point]
      println( "Get group : " + group );
      db.createDomain( domName, group );
      db.createDomain( domNameSYS, ["SYSCatalogGroup"] );
   }
   catch( e )
   {
      if( -278 != e )
      {
         println( "Failed to create domain, rc = " + e );
         throw e;
      }
      else
         println( "Cannot create domain specify \"SYSCatalogGroup\"" );
   }

   // Inspect domain
   inspectDomain( db, domName );
   println( "Success to create domain : [" + domName + "]" );

   // Create CS in domain and create collection
   try
   {
      commCreateCS( db, csName, false, "create CS specify domain",
         { "Domain": domName } );
      commCreateCL( db, csName, clName, {}, false, false,
         "create collection in domain" );
   }
   catch( e )
   {
      println( "Failed to create CS by specify domain, rc = " + e );
      throw e;
   }

   // Insert data
   insertData( db, csName, clName, 1000 );

   // Query data
   queryData( db, csName, clName );

   // Update data
   updateData( db, csName, clName );

   // Remove data
   removeData( db, csName, clName );

   // Clear domain in the end
   clearDomain( db, domName );
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
