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

function removeSlaverNode ( db, group )
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
      var slaveNodeHostname = getSlaverHostname( dataRG );
      var slaveNodeSvcname = getSlaverSvcname( dataRG );
      dataRG.removeNode( slaveNodeHostname, slaveNodeSvcname );
   }
   catch( e )
   {
      throw buildException(
         "remove slave node",
         e,
         "dataRG.removeNode( " + slaveNode + " ); ",
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
      removeSlaverNode( db, groupName );
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