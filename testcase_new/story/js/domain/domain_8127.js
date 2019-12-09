/******************************************************************************
@Description : 1. Test db.listDomains(<name>,[option]), list all domains.
               2. Test insert/update/find/remove operation.
               3. Test create four domains.
               4. Test listDomains(), where domain name is no exist.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domNames = new Array( CHANGEDPREFIX + "DomTest1", CHANGEDPREFIX + "DomTest2",
      CHANGEDPREFIX + "DomTest3", CHANGEDPREFIX + "DomTest4" );

   // Drop all domains, if have
   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
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
      println( "Success to create domain : [" + domNames + "]" );
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

   // List not exit domain name [Testing Point]
   try
   {
      var listDom = db.listDomains( { "Name": "NotExistDomName" } );
      var notDomName = listDom.current().toObj()["Name"];
   }
   catch( e )
   {
      if( -29 != e )
      {
         println( "Failed to list domains, rc = " + e );
         throw e;
      }
      else
         println( "Wrong to list not exist domain" );
   }

   // List all domains and inspect[Testing Point]
   listDom = db.listDomains();
   listDomArray = new Array();
   while( listDom.next() )
   {
      listDomArray.push( listDom.current().toObj()["Name"] );
   }
   // Inspect the domains
   for( var i = 0; i < domNames.length; ++i )
   {
      for( var j = 0; j <= listDomArray.length; ++j )
      {
         if( listDomArray[j] == domNames[i] )
         {
            break;
         }
         if( j == listDomArray.length )
         {
            println( "Don't have domnames, domains = [" + domNames +
               "] ... [" + listDomArray + "]" );
            throw "ErrDomains";
         }
      }
   }
   println( "Success to list domains" );

   for( var i = 0; i < domNames.length; ++i )
   {
      // Inspect domain
      inspectDomain( db, domNames[i] );

      // Drop domain int the end
      clearDomain( db, domNames[i] );

   }
   // Clear Env
   println( "Clear domains = [" + domNames + "]" );
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
