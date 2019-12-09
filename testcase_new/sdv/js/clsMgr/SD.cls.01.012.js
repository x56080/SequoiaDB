/* *****************************************************************************
@Description: build multiple new data group
@author:      LY
***************************************************************************** */

function createDataGroup ( db, port, num )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === port ) 
   {
      throw ( "the port is null, error" );
   }
   if( undefined === num ) 
   {
      num = 2;
   }

   var datagroup = CHANGEDPREFIX + "_group" + num;
   try
   {
      var dataRG = db.createRG( datagroup );
      dataRG.createNode( COORDHOSTNAME, port, SPAREPORTPATH + port );
      dataRG.start();
      return datagroup;
   }
   catch( e )
   {
      throw buildException(
         "build data node",
         e,
         "dataRG.createNode(" + COORDHOSTNAME + ", " + port + ", " + SPAREPORTPATH + port + "); dataRG.start()",
         "succ",
         "failure"
      );
   }
}

function main ( db )
{
   try
   {
      // create data node port
      var data = generatePort( SPAREPORTSTART );
      var num = 2;
      if( !commIsStandalone( db ) )
      {
         for( var i = 0; i < num; ++i )
         {
            data = generatePort( data );
            var groupName = createDataGroup( db, data, i )
            //build collection
            var clName = COMMCLNAME + "_" + i;
            // clean collection in the beginning
            commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
            try
            {
               //build collection
               var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName }, false, true, "create collection" );
               // insert data and check result
               insertAndCheck( cl, 10000, "check collection records in the end, error" );
               // clean collection after test correct
               commDropCL( db, COMMCSNAME, clName, false, false, "clear collection in the end, correct" );
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