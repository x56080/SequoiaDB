/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), before group have data.
               2. Test the data/idx in domain groups.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomDataGroupData";
   // Drop CS
   commDropCS( db, csName, clName, true,
      "clear collection space in the beginning" );

   // Clear domain in the begnning
   clearDomain( db, domName );

   // Data operation before create domain [Testing Point 1]
   try
   {
      println( "Begin to operation data before create domain" );
      // Create Collection Space
      commCreateCS( db, csName, false, "create CS specify domain" );

      // Create Collection
      commCreateCL( db, csName, clName, {}, false, false,
         "create collection in domain" );

      // Create index
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      cl.createIndex( "befDomIdx", { "No": 1 }, true, true );

      // Insert data
      insertData( db, csName, clName, 1000 );

      // Query data
      queryData( db, csName, clName );

      // Update data
      updateData( db, csName, clName );

      // Remove data
      removeData( db, csName, clName );
   }
   catch( e )
   {
      println( "Failed to Data Run before create domain, rc = " + e );
      throw e;
   }

   // Get collection space located in group
   var csGroup = getCSGroup( db, csName, clName );

   // Create Domain specify this Group and inspect
   db.createDomain( domName, [csGroup] );
   //db.createDomain( domName, [ "group1" ] ) ;
   inspectDomain( db, domName );

   // After create domain, create collection space and collection
   var csname = csName + "_DomAfter";
   var clname = clName + "_DomAfter";
   try
   {
      commCreateCS( db, csname, false, "create CS specify domain",
         { "Domain": domName } );
      commCreateCL( db, csname, clname, {}, false, false,
         "create collection in domain" );
      println( "Create CS and CL over" );

      // Create index
      var CS = db.getCS( csname );
      var CL = CS.getCL( clname );
      CL.createIndex( "aftDomIdx", { "cardID": -1 } );

      // Insert data
      insertData( db, csname, clname, 1000 );

      // Query data
      queryData( db, csname, clname );

      // Update data
      updateData( db, csname, clname );

      // Remove data
      removeData( db, csname, clname );
   }
   catch( e )
   {
      println( "Failed to Operation Sdb after create domain, rc = " + e );
      throw e;
   }

   // Drop domain int the end
   clearDomain( db, domName );

   // Drop CS created before Create domain
   commDropCS( db, csName, clName, true,
      "clear collection space in the end" );

}

try
{
   println( "Begin to run" );
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
