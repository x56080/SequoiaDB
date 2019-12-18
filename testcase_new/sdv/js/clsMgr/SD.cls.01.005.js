/* *****************************************************************************
@Description: Deployment cluster mode
@author:      LY
***************************************************************************** */

function createCatalog ( tmpDb, hostname )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }

   // build catalog group and node
   var catalog = generatePort( SPAREPORTSTART );
   try
   {
      tmpDb.createCataRG( hostname, catalog, SPAREPORTPATH + catalog );
      return catalog;
   }
   catch( e )
   {
      println( "Enable catalog failed: " + e );
      buildException(
         "build catalog",
         e,
         "db.createCataRG( " + COORDHOSTNAME + ", " + catalog + ", " + SPAREPORTPATH + catalog + " )",
         "succ",
         "failure"
      );
   }
}

function createData ( tmpDb, hostname )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }

   // build data group and node
   var data = generatePort( SPAREPORTSTART );
   try
   {
      var dataRG = tmpDb.createRG( "new_datagroup" );
      dataRG.createNode( hostname, data, SPAREPORTPATH + data );
      dataRG.start();
      return data;
   }
   catch( e )
   {
      println( "Enable data node failed: " + e );
      buildException(
         "build data node",
         e,
         "dataRG.createNode( " + COORDHOSTNAME + ", " + data + ", " + SPAREPORTPATH + data + " ); dataRG.start()",
         "succ",
         "failure"
      );
   }
}

function createCoord ( tmpDb, hostname )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }

   // build coord group and node
   var coord = generatePort( SPAREPORTSTART );
   try
   {
      var coordRG = tmpDb.createCoordRG();
      coordRG.createNode( hostname, coord, SPAREPORTPATH + coord );
      coordRG.start();
      return coord;
   }
   catch( e )
   {
      println( "Enable coord failed: " + e );
      buildException(
         "build coord node",
         e,
         "coordRG.createNode( " + COORDHOSTNAME + ", " + coord + ", " + SPAREPORTPATH + coord + "); dataRG.start()",
         "succ",
         "failure"
      );
   }
}

function deploySdb ( tmpDb, hostname, port )
{
   if( undefined === tmpDb ) 
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

   // common database connection
   try
   {
      var db = new Sdb( hostname, port );
      return db;
   }
   catch( e )
   {
      println( "Connect Failed in deploySdb function!" );
      buildException(
         "connect coord",
         e,
         "var db = new Sdb(" + hostname + ", " + coord + ")",
         "succ",
         "failure"
      );
   }
}

function checkResult ( db )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }

   // clear collection in the beginning
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning" );
   try
   {
      //build collection
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false, "create collection in the beginning" );
      // insert data and check result
      insertAndCheck( cl, 10000, "check collection records in the end, error" );
   }
   catch( e )
   {
      throw buildException(
         "build collection and insert records",
         e,
         "collection name : " + cl,
         "succ",
         "failure"
      );
   }
   finally
   {
      // clear collection after test correct
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
      db.close();
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
      // build temp coord and connect it
      var tmpDb = connectSdb( COORDHOSTNAME, CMSVCNAME, tmpCoordPort, false );
      // create catalog group 
      var catalog = createCatalog( tmpDb, hostname );
      // create coord group 
      var data = createData( tmpDb, hostname );
      // create data group 
      var coord = createCoord( tmpDb, hostname );
      // deploy new cluster
      var newDB = deploySdb( tmpDb, hostname, coord );
      // insert records and check it
      checkResult( newDB );
      // clear data after test correct
      clean( tmpDb, hostname, data, "new_datagroup", "data_group" );
      // clear coord after test correct
      clean( tmpDb, hostname, coord, "", "coord_group" );
      // clear catalog after test correct
      clean( tmpDb, hostname, catalog, "", "catalog_group" );
      // clear temp coord after test correct
      clean( tmpDb, hostname, tmpCoordPort, "", "tmp_coord" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      tmpDb.close();
      db.close();
   }
}


// run main
main( db );