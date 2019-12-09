/******************************************************************************
@Description : 1. Test create domain, specify Name "SYSDOMAIN" [Error].
               2. Test create domain specify nothing[NoArg].
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomSYSDOMAIN";

   // Drop domain in the begnning
   clearDomain( db, domName );

   // 1. Create domain, { Name : "SYSDOMAIN" }[ERR] [Testing Point]
   try
   {
      db.createDomain( domName );
      // cannot create domain specify "SYSDOMAIN"
      db.createDomain( "SYSDOMAIN" );
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "Failed to create domain, rc = " + e );
         throw e;
      }
      else
         println( "Create domain cannot use domain name : SYSDOMAIN" );
   }
   // Inspect domain
   inspectDomain( db, domName );
   println( "Success to create domain : [" + domName + "]" );

   // 2. Create domain don't specify parameter[ERR] [Testing Point]
   try
   {
      db.createDomain();
   }
   catch( e )
   {
      if( -259 != e )
      {
         println( "create domain must specify parameter, rc = " + e );
         throw e;
      }
   }

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
