/**********************************************
@Description: remove the single data node
@author:      LY
**********************************************/

function createDataNode ( db, hostname, port, group, num )
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
   if( undefined === group ) 
   {
      throw ( "the group is null, error" );
   }

   try
   {
      var dataRG = db.createRG( group );
      for( var i = 0; i < num; i++ )
      {
         port = generatePort( port );
         dataRG.createNode( hostname, port, SPAREPORTPATH + port );
      }
      dataRG.start();
   }
   catch( e )
   {
      throw buildException(
         "build data node",
         e,
         "var dataRG = db.createRG( " + group + " ); dataRG.createNode( " + hostname
         + ", " + port + ", " + SPAREPORTPATH + port + " ); dataRG.start()",
         "succ",
         e
      );
   }
   finally
   {
      sleep( 10 );
      var masterNode = dataRG.getMaster();
      return masterNode;
   }
}

function splitAndCheck ( db, originalGroup, newGroup, node )
{
   // clean collection in the beginning
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning of split" );
   try
   {
      //build collection
      var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, { ShardingKey: { "no": 1 }, ShardingType: "range", ReplSize: 0, Compressed: true, Group: originalGroup }, false, true, "create collection in the beginning of split" );
      // insert data and check result
      insertAndCheck( cl, 10000, true, true, "check collection records in the end, error" );
      // split data from original data group to creatting data group
      cl.split( originalGroup, newGroup, 50 );
      // check result
      checkDataNode( 5000, node );
      // clean collection after test correct
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
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
}

function checkDataNode ( num, node )
{
   if( undefined == num ) 
   {
      num = 1;
   }

   var newDb = node.connect();
   var cl = newDb.getCS( COMMCSNAME ).getCL( COMMCLNAME );
   var size = 0;
   try
   {
      size = cl.count();
      if( size.toString() !== num )
      {
         println( "Count all size : " + size + " is not same with " + num );
         throw "count is not equel, check failed";
      }
   }
   catch( e )
   {
      println( "Count all exception: " + e );
      throw buildException( "cl count", e, "cl.count()", num, size );
   }
   finally
   {
      newDb.close();
   }
}

function main ( db )
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      // create data node port
      var data = generatePort( SPAREPORTSTART );
      // create dataGroup name
      var groupName = CHANGEDPREFIX + "_group";
      // get new cluster used hostname 
      var hostname = getHostName();
      // create new data node in the original data group
      var masterNode = createDataNode( db, hostname, data, groupName, 3 );
      // split data from original data group to creatting data group and check out results
      splitAndCheck( db, "group1", groupName, masterNode );
      // clean 
      clean( db, hostname, data, groupName, "data_group" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}

//run main
main( db );