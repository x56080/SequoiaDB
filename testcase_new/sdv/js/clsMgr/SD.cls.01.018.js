/* *****************************************************************************
@Description: create new crood node in the original crood group
@author:      LY
***************************************************************************** */

function createCoordNode ( db, hostname, port )
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
      var coordRG = db.getCoordRG();
      var coordNode = coordRG.createNode( hostname, port, SPAREPORTPATH + port );
      coordNode.start();
   }
   catch( e )
   {
      throw buildException(
         "build crood node",
         e,
         "var coordNode = coordRG.createNode( " + hostname + ", " + port + ", " + SPAREPORTPATH + port + " ); coordNode.start()",
         "succ",
         "failure"
      );
   }

   // common database connection
   try
   {
      var newDb = coordNode.connect();
      return newDb;
   }
   catch( e )
   {
      throw buildException(
         "connect coord",
         e,
         "var newDb = new Sdb( " + hostname + ", " + port + " )",
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
      // clear collection after test correct
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
   }
   catch( e )
   {
      throw buildException(
         "build collection and insert records",
         e,
         "collection name : " + cl,
         "succ",
         "failure" );
   }
}

function main ( db )
{
   try
   {
      // create coord node port
      var coord = generatePort( SPAREPORTSTART );
      // get new cluster used hostname 
      var hostname = getHostName();
      if( !commIsStandalone( db ) )
      {
         // create new coord node in the original coord group
         var newDb = createCoordNode( db, hostname, coord );
         // check new coord node exist or not
         checkResult( newDb, coord );
         // clean data after test correct
         clean( db, hostname, coord, "", "coord" );
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
      newDb.close();
      db.close();
   }
}

// Run Main
main( db );