/* *****************************************************************************
@Description: create new data node in the original data group
@author:      LY
***************************************************************************** */

function createDataNode ( db, hostname, port, group )
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
      var dataRG = db.getRG( group );
      var dataNode = dataRG.createNode( hostname, port, SPAREPORTPATH + port );
      dataNode.start();
   }
   catch( e )
   {
      throw buildException(
         "build data node",
         e,
         "var dataNode = dataRG.createNode( " + hostname + ", " + port + ", " + SPAREPORTPATH + port + " ); dataNode.start()",
         "succ",
         "failure"
      );
   }
   finally
   {
      return dataNode;
   }
}

function checkResult ( db, hostname, group, node )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }
   if( undefined === group ) 
   {
      throw ( "the group is null, error" );
   }

   // clear collection in the beginning
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning" );
   try
   {
      //build collection
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, { Group: group, ReplSize: 0 }, true, false, "create collection" );
      // insert data and check result
      insertAndCheck( cl, 10, "check collection records in the end, error" );
      var groups = commGetGroups( db, "", group );
      while( true )
      {
         var errNodes = commCheckBusiness( groups, true );
         if( errNodes.length == 0 )
         {
            break;
         }
         sleep( 1 );
      }
      var dataNodeOld = new Sdb( hostname, "21100" );
      var numParallel = dataNodeOld.getCS( COMMCSNAME ).getCL( COMMCLNAME ).count();

      var dataNodeNew = node.connect();
      var numDetect = dataNodeNew.getCS( COMMCSNAME ).getCL( COMMCLNAME ).count();
      if( parseInt( numParallel ) != parseInt( numDetect ) )
      {
         throw buildException(
            "check two date node records",
            "the two date node records is not equal",
            "",
            "equal",
            "not equal"
         );
      }
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
      dataNodeOld.close();
      dataNodeNew.close();
   }
}

function main ( db )
{
   try
   {
      // create data node port
      var data = generatePort( SPAREPORTSTART );
      // create new data group name
      var groupName = "group1";
      // get new cluster used hostname 
      var hostname = getHostName();
      if( !commIsStandalone( db ) )
      {
         // create new data node in the original data group
         var dataNode = createDataNode( db, hostname, data, groupName );
         // check new data node and original data node
         checkResult( db, hostname, groupName, dataNode );
         // clean data after test correct
         clean( db, hostname, data, groupName, "data" );
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