/******************************************************************************
@Description : 1. Test db.listDomains(<name>,[option]), specify name list.
               2. Test insert/update/find/remove operation.
               3. Test create four domains.
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
      println( "Success to create Domain : [" + domNames + "]" );
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

   // Inspect the specify domain
   for( var j = 0; j <= domNames.length; ++j )
   {
      var listDom = db.listDomains( { "Name": domNames[j] }
      ).current().toObj()["Name"];
      //println( "list domain" + listDom ) ;
      if( listDom == domNames[j] )
      {
         break;
      }
      else
      {
         println( "Don't have domnames, domains = [" + domNames[j] +
            "] ... [" + listDom + "]" );
         throw "ErrDomains";
      }
   }

   for( var i = 0; i < domNames.length; ++i )
   {
      // Inspect domain
      inspectDomain( db, domNames[i] );

      // Drop domain int the end
      clearDomain( db, domNames[i] );
   }
   // Clear Env
   println( "Success to drop domains = [" + domNames + "]" );
}

try
{
   if( false == commIsStandalone( db ) )
      main( db );
   else
      println( "run mode : standalone" );
   db.close();
}
catch( e )
{
   throw e;
}
