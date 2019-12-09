/* *****************************************************************************
@Description: build new data group
@author:      LY
***************************************************************************** */

function main ( db )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }

   try
   {
      // create data node port
      var data = generatePort( SPAREPORTSTART );
      // create dataGroup name
      var dataGroup = CHANGEDPREFIX + "_group";
      // get local machine hostname 
      var hostname = getHostName();
      if( !commIsStandalone( db ) )
      {
         try
         {
            var dataRG = db.createRG( dataGroup );
            dataRG.createNode( hostname, data, SPAREPORTPATH + data );
            dataRG.start();
         }
         catch( e )
         {
            println( "Enable data node failed: " + e );
            throw buildException(
               "build data node",
               e,
               "dataRG.createNode( " + hostname + ", " + data + ", " + SPAREPORTPATH + data + "); dataRG.start()",
               "succ",
               "failure"
            );
         }
         // clean collection in the beginning
         commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning" );
         try
         {
            //build collection
            var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, { Group: dataGroup }, false, true, "create collection in the beginning" );
            // insert data and check result
            insertAndCheck( cl, 10000, "check collection records in the end, error" );
            // clean collection after test correct
            commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
            // clean data group after test correct
            clean( db, hostname, data, dataGroup, "data_group" );
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