/* *****************************************************************************
@Description: create new catalog node in the original catalog group
@author:      LY
***************************************************************************** */

function createCatalogNode ( db, hostname, port )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }
   if( undefined === port ) 
   {
      throw ( "the port is null, error" );
   }

   try
   {
      var catalogRG = db.getCatalogRG();
      var catalogNode = catalogRG.createNode( hostname, port, SPAREPORTPATH + port );
      catalogNode.start();
   }
   catch( e )
   {
      buildException(
         "build catalog node",
         e,
         "var catalogNode = catalogRG.createNode( " + hostname + ", " + port + ", " + SPAREPORTPATH + port + " ); catalogNode.start()",
         "succ",
         "failure"
      );
   }
}

function checkResult ( db, port )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === port ) 
   {
      throw ( "the port is null, error" );
   }

   try
   {
      var catalogGroupObj = db.getCatalogRG().getDetail().current().toObj();
      var status = catalogGroupObj["Status"];
      if( 1 !== status )
      {
         buildException(
            "check catalog group status",
            "the catalog group status is not running",
            "",
            "running",
            "not running"
         );
      }
      var num = catalogGroupObj["Group"].length;
      var counter = 1;
      for( var i = 0; i < num; i++ )
      {
         var catalogPort = catalogGroupObj["Group"][i]["Service"][0]["Name"];
         if( port !== catalogPort )
         {
            counter++;
         }
      }
      if( num !== counter )
      {
         buildException(
            "check new catalog node exist",
            "the new catalog node is not exist",
            "",
            "exist",
            "not exist"
         );
      }
   }
   catch( e )
   {
      buildException(
         "build collection and insert records",
         e,
         "collection name : " + COMMCLNAME,
         "succ",
         "failure"
      );
   }
}

function main ( db )
{
   try
   {
      // create catalog node port
      var catalog = generatePort( SPAREPORTSTART );
      // get new cluster used hostname 
      var hostname = getHostName();
      if( !commIsStandalone( db ) )
      {
         // create new catalog node in the original catalog group
         createCatalogNode( db, hostname, catalog );
         // check new catalog node exist or not
         checkResult( db, catalog );
         // clean catalog after test correct
         clean( db, hostname, catalog, "", "catalog" );
      }
      else
      {
         println( "run mode is standalone" );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      db.close();
   }
}

// Run Main
main( db );