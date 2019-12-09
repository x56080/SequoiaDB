/* *****************************************************************************
@Description: build crood group and check its status
@author:      LY
***************************************************************************** */

function deployCatalog ( tmpDb, hostname )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the tmpDb is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }

   // build catalog
   var catalog = generatePort( SPAREPORTSTOP );
   try
   {
      tmpDb.createCataRG( hostname, catalog, SPAREPORTPATH + catalog );
      return catalog;
   }
   catch( e )
   {
      throw buildException(
         "build catalog",
         e,
         "db.createCataRG( " + COORDHOSTNAME + ", " + catalog + ", " + SPAREPORTPATH + catalog + " )",
         "succ",
         "failure"
      );
   }
}

function deploySdb ( tmpDb, hostname, port )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the tmpDb is null, error" );
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
      var coordRG = tmpDb.createCoordRG();
      coordRG.createNode( hostname, port, SPAREPORTPATH + port );
      coordRG.start();
   }
   catch( e )
   {
      println( "Enable coord failed: " + e );
      throw buildException(
         "build coord node",
         e,
         "coordRG.createNode( " + COORDHOSTNAME + ", " + port + ", " + SPAREPORTPATH + port + "); coordRG.start()",
         "succ",
         "failure"
      );
   }

   // common database connection
   try
   {
      var db = new Sdb( hostname, port );
      return db;
   }
   catch( e )
   {
      println( "Connect Failed in Common Function!" );
      buildException(
         "connect coord",
         e,
         "var db = new Sdb( " + hostname + ", " + port + " )",
         "succ",
         "failure"
      );
   }
}

function checkResult ( db )
{
   var ret = db.getCoordRG().getDetail().current()
   if( 1 !== ret.toObj()["Status"] )
   {
      buildException(
         "check coord status",
         "coord status error",
         "ret = db.getCoordRG().getDetail().current().toObj()['Status'] not equal 1",
         1,
         ret.toObj()['Status']
      );
   }
}

function main ( db )
{
   try
   {
      // generate temp coord port
      var tmpCoordPort = generatePort( SPAREPORTSTART );
      // get new cluster used hostname 
      var hostname = getHostName();
      // create coord node port
      var coord = generatePort( tmpCoordPort );
      // build temp coord and connect it
      var tmpDb = connectSdb( COORDHOSTNAME, CMSVCNAME, tmpCoordPort, false );
      // create new catalog group
      var catalog = deployCatalog( tmpDb, hostname );
      // create new coord node and connect it
      var newDb = deploySdb( tmpDb, hostname, coord );
      // check the new coord group status
      checkResult( newDb );
      // clean coord after test correct
      clean( tmpDb, hostname, coord, "", "coord_group" );
      // clean catalog after test correct
      clean( tmpDb, hostname, catalog, "", "catalog_group" );
      // clean temp coord after test correct
      clean( tmpDb, hostname, tmpCoordPort, "", "tmp_coord" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      newDb.close();
      tmpDb.close();
      db.close();
   }
}


// run main
main( db );