/******************************************************************************
@Description : 1. Test db.getDomain(<name>), specify name list.
               2. Test get domain , domain name don't exist.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomFuncGetDomain";

   // Clear domain in the benignning
   clearDomain( db, domName );
   println( "Clear domain in the beginning" );

   // Get all data groups and create domain by specify AutoSplit
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      createDomain( db, domName, group );
      println( "Success to create domain" );
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

   // Get domain that not exsit [Testing Point]
   try
   {
      var noDom = db.getDomain( "SYSDOMAINGetNotExsitDomain" );
   }
   catch( e )
   {
      if( -214 != e )
      {
         println( "Failed to get domain, rc = " + e );
         throw e;
      }
      else
         println( "Don't have domain : GetNotExsitDomain" );
   }

   // Get domain and list collection and collectionspace [Testing Point]
   var dom = db.getDomain( domName );
   if( dom != domName )
   {
      println( "Failed to get domain, domain = " + domName );
      throw "ErrGetDom";
   }

   // Inspect domain
   inspectDomain( db, domName );

   // Drop domain int the end
   clearDomain( db, domName );
   println( "Clear domain in the end" );

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
