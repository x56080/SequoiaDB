/**********************************************
@Description: remove the single data node
@author:      LY
**********************************************/

function createDataGroup ( db, hostname, port, group, num )
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
         dataRG.createNode( hostname, port, SPAREPORTPATH + port );
         port = generatePort( port );
      }
      dataRG.start();
   }
   catch( e )
   {
      throw buildException(
         "build data node",
         e,
         "var dataRG = db.createRG( " + group + " ); dataRG.createNode( " + hostname +
         ", " + port + ", " + SPAREPORTPATH + port + " ); dataRG.start()",
         "succ",
         e
      );
   }
}

function removeMasterNode ( db, group )
{
   try
   {
      while( true )
      {
         var groups = commGetGroups( db, "", group );
         if( -1 !== groups[0][0]["PrimaryNode"] )
         {
            break;
         }
         sleep( 1 );
      }
      var dataRG = db.getRG( group );
      var masterNodeHostname = getMasterHostname( dataRG );
      var masterNodeSvcname = getMasterSvcname( dataRG );
      dataRG.removeNode( masterNodeHostname, masterNodeSvcname );
      throw buildException(
         "remove master node",
         e,
         "dataRG.removeNode( " + masterNode + " ); ",
         -204,
         "succ"
      );
   }
   catch( e )
   {
      if( -204 !== e )
      {
         throw buildException(
            "remove master node",
            e,
            "dataRG.removeNode( " + masterNode + " ); ",
            -204,
            e
         );
      }
   }
   finally
   {
      var masterNode = new Array();
      masterNode[0] = masterNodeHostname;
      masterNode[1] = masterNodeSvcname;
      return masterNode;
   }
}

function changeMasterNode ( db, group, masterNode )
{
   try
   {
      while( true )
      {
         var groups = commGetGroups( db, "", group );
         if( -1 !== groups[0][0]["PrimaryNode"] )
         {
            break;
         }
         sleep( 1 );
      }
      var dataRG = db.getRG( group );
      var dataNode = dataRG.getNode( masterNode[0], masterNode[1] );
      dataNode.stop();
      dataNode.start();

      while( true )
      {
         var groups = commGetGroups( db, "", group );
         if( -1 !== groups[0][0]["PrimaryNode"] )
         {
            break;
         }
         sleep( 1 );
      }
      var newMasterNodeHostname = getMasterHostname( dataRG );
      var newMasterNodeSvcname = getMasterSvcname( dataRG );
      if( masterNode[0] === newMasterNodeHostname && masterNode[1] === newMasterNodeSvcname )
      {
         changeMasterNode( db, group, masterNode );
      }
   }
   catch( e )
   {
      throw buildException(
         "changeMasterNode master node",
         e,
         "dataRG.stopNode( " + masterNode[0] + ", " + masterNode[1] + " ); dataRG.startNode( "
         + masterNode[0] + ", " + masterNode[1] + " );",
         "succ",
         "failure"
      );
   }
}

function removeOriginMasterNode ( db, group, masterNode )
{
   try
   {
      var dataRG = db.getRG( group );
      dataRG.removeNode( masterNode[0], masterNode[1] );
   }
   catch( e )
   {
      throw buildException(
         "remove master node",
         e,
         "dataRG.removeNode( " + masterNode[0] + ", " + masterNode[1] + " ); ",
         "succ",
         "failure"
      );
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
      createDataGroup( db, hostname, data, groupName, 3 );
      var masterNode = removeMasterNode( db, groupName );
      // split data from original data group to creatting data group and check out results
      changeMasterNode( db, groupName, masterNode );
      removeOriginMasterNode( db, groupName, masterNode );
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