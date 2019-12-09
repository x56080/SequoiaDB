/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), have name and Groups.
               2. Test create CS on the domain.
               3. Test insert/update/find/remove operation.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var DOMCSNAME = CHANGEDPREFIX + "_domFoo";
   var domainName = CHANGEDPREFIX + "_domainNameAndGroup"

   // Drop CS
   commDropCS( db, DOMCSNAME, COMMCLNAME, true,
      "clear collection space in the beginning" );

   // Drop domain
   clearDomain( db, domainName );

   // Get one group and create domain [Testing Point]
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Groups" + group[1] ) ;
      db.createDomain( domainName, [group[0]] );
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

   // Inspect domain that was created
   inspectDomain( db, domainName );

   // Create CS in domain and create collection [Testing Point]
   try
   {
      commCreateCS( db, DOMCSNAME, false, "create CS specify domain",
         { "Domain": domainName } );
      commCreateCL( db, DOMCSNAME, COMMCLNAME, -1, true, false, false,
         "create collection in domain" );
   }
   catch( e )
   {
      println( "Failed to create CS by specify domain, rc = " + e );
      throw e;
   }

   // Insert data
   insertData( db, DOMCSNAME, COMMCLNAME, 1000 )
   println( "Success to insert 2000 records in database" );

   // Query data
   queryData( db, DOMCSNAME, COMMCLNAME )

   // Update data
   updateData( db, DOMCSNAME, COMMCLNAME )

   // Remove data
   removeData( db, DOMCSNAME, COMMCLNAME )

   // Drop domain int the end
   clearDomain( db, domainName );
   println( "Success to clean domain : [" + domainName + "] in the end" );
}

try
{
   if( false == commIsStandalone( db ) )
   {
      main( db );
   }
   else
   {
      println( "Run Mode is Standalone." );
   }
   db.close();
}
catch( e )
{
   db.close();
   throw e;
}
