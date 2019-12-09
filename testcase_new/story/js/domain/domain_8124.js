/******************************************************************************
@Description : 1. Test db.dropDomains(<name>), specify name .
               2. Test create four domains.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   domname1 = "DomTest_1" + COMMCLNAME;
   domname2 = "DomTest_2" + COMMCLNAME;
   domname3 = "DomTest_3" + COMMCLNAME;
   domname4 = "DomTest_4" + COMMCLNAME;
   // Inspect run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   domNames = new Array( domname1, domname2, domname3, domname4 );

   // Drop all domains, if have
   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
      //println( "Clear :"+domName[i]+"count :"+i ) ;
   }
   println( "Clear domain in the beginning" );

   // Get all data groups and create domain by specify AutoSplit
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      for( var i = 0; i < domNames.length; ++i )
      {
         createDomain( db, domNames[i], group );
      }
      println( "Success to create domain, domain : [" + domNames + "]" );
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

   for( var i = 0; i < domNames.length; ++i )
   {
      // Inspect domain
      inspectDomain( db, domNames[i] );

      // Drop domains that were created
      db.dropDomain( domNames[i] );
      //println( "Drop domain over" ) ;

      // Inspect domains
      try
      {
         inspectDomain( db, domNames[i] );
      }
      catch( e )
      {
         if( "NoDomain" == e )
         {
            println( "Success to drop domain : [ " + domNames[i] +
               " ] in the end" );
         }
         else
            throw e;
      }
   }
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
