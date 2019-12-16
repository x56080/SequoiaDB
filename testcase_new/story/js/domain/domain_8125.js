/******************************************************************************
@Description : 1. Test db.dropDomains(<name>), specify not exist name .
               2. Test create four domains.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomDropNotExistName";
   // Drop CS
   commDropCS( db, csName, clName, true,
      "clear collection space in the beginning" );

   // Drop the domain we specify in the begnning
   clearDomain( db, domName )

   // Get all data groups and create domain by specify AutoSplit
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      createDomain( db, domName, group );
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


   // Drop not exist domain [Testing Point]
   try
   {
      db.dropDomain( "SYSDOMAIN" );
   }
   catch( e )
   {
      if( -214 != e )
      {
         println( "Failed to dropDomain, rc = " + e );
         throw e;
      }
      else
         println( "Don't have domain : SYSDOMAIN" );
   }

   // Drop domain where CS/CL/data record in [Testing Point]
   try
   {
      db.dropDomain( domName );
   }
   catch( e )
   {
      if( -256 != e )
      {
         println( "Faild to drop Domain, rc = " + e );
         throw e;
      }
      else
         println( "Domain is not empty, cannot drop" );
   }

   // Inspect domain
   inspectDomain( db, domName );

   // Drop CS int the end
   commDropCS( db, csName, clName, true,
      "clear collection space in the end" );

   // Drop domain in the end
   clearDomain( db, domName );
   println( "Success to clear domain : [" + domName + "]" );
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
