/* *****************************************************************************
@Description: create duplicate data group
@author:      LY
***************************************************************************** */

function createDataGroup ( db, group )
{
   if( undefined === db ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === group ) 
   {
      throw ( "the group is null, error" );
   }

   try
   {
      var dataRG = db.createRG( group );
   }
   catch( e )
   {
      if( -153 !== e )
         throw buildException(
            "build data group",
            e,
            "var dataRG = db.getcreateRG( " + group + " ) ;",
            -153,
            e
         );
   }
}

function main ( db )
{
   try
   {
      // get original data group name
      var groupName = "group1";
      if( !commIsStandalone( db ) )
      {
         // create new data node in the original data group
         createDataGroup( db, groupName );
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