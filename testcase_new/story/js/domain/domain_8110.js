/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), only have domain name.
               2. Test "NULL domain" cannot be created collection space.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var DOMCSNAME = CHANGEDPREFIX + "_standalone";
   var domainName = CHANGEDPREFIX + "_domainNameOnly"
   if( true == commIsStandalone( db ) )
   {
      // Create Domain in standalone
      println( "<Rum Mode is Standalone>" );
      try
      {
         db.createDomain( domainName );
         throw "ErrRun";
      }
      catch( e )
      {
         if( -159 != e )
         {
            println( "Error to run createDomain in Standalone, rc = " + e );
            throw e;
         }
      }

      // drop CS
      try
      {
         db.dropCS( DOMCSNAME );
      }
      catch( e )
      {
         if( -34 != e )
         {
            println( "Failed to drop CS, rc = " + e );
            throw e;
         }
      }

      // Create CS attach Domain in standalone
      try
      {
         db.createCS( DOMCSNAME, { 'Domain': domainName } );
         //throw "ErrRun" ;
      }
      catch( e )
      {
         if( -159 != e )
         {
            println( "Error to run createCS in Standalone, rc = " + e );
            throw e;
         }
      }
      throw "RunMode_StandAlone";
   }

   // Drop CS in the beginning
   commDropCS( db, DOMCSNAME, true, "create CS in the beginning" );

   // Drop domain in the beginning
   clearDomain( db, domainName );

   // Create domain only have domainName [Testing Point]
   try
   {
      db.createDomain( domainName );
      println( "create domain = " + domainName );
   }
   catch( e )
   {
      println( "Failed to create domain, rc = " + e );
      throw e;
   }

   // Inspect domain
   inspectDomain( db, domainName );

   // Create collection space in this domain,
   // "NULL domain" cannot be created CS [Testing Point]
   try
   {
      db.createCS( csName, { "Domain": domainName } )
   }
   catch( e )
   {
      if( -262 != e )  // domain does not have any groups at all
      {
         println( "Failed to create CS by specify domain, rc = " + e );
         throw e;
      }
   }

   // Drop domain int the end
   clearDomain( db, domainName );
   println( "Success to clean domain : [" + domainName + "] in the end" );
}

try
{
   main( db );
   db.close();
}
catch( e )
{
   switch( e )
   {
      case "ErrRun":
         println( "Shouldn't create CS or Domain in standalone" );
         throw e;
         break;
      case "RunMode_StandAlone":
         println( "Run Mode is : [ Standalone ] " );
         break;
      default:
         throw e
   }
}
