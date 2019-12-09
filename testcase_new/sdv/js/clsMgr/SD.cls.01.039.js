/* *****************************************************************************
@Description: create duplicate data node in the original data group on the same port
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
      dataRG.createNode( hostname, port, SPAREPORTPATH + port );
   }
   catch( e )
   {
      if( -157 !== e )
         throw buildException(
            "build data node",
            e,
            "var dataRG = db.getRG( " + group + " ); dataRG.createNode( " + hostname
            + ", " + port + ", " + SPAREPORTPATH + port + " )",
            -153,
            e
         );
   }
}

function main ( db )
{
   try
   {
      // get duplicate data node port
      var data = "21100";
      // get original data group name
      var groupName = "group1";
      // get new cluster used hostname 
      var hostname = getHostName();
      if( !commIsStandalone( db ) )
      {
         // create new data node in the original data group
         createDataNode( db, hostname, data, groupName );
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