/*******************************************************************************
*@Description : common functions
*@Modify list :
*              2016/7/11 huangxiaoni
*******************************************************************************/
function isTransautocommit ()
{
   var isTAC = false;
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { "role": "data" } );
   while( cursor.next() ) 
   {
      var rc = cursor.current().toObj();
      if( rc.transautocommit === 'TRUE' )
      {
         isTAC = true;
      }
      return isTAC;
   }
}

function createCL ( csName, clName, autoCreateCS, ignoreExisted, message )
{
   println( "\n---Begin to create CL." );

   if( autoCreateCS == undefined ) { autoCreateCS = true; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }

   //createCS
   if( autoCreateCS )
   {
      commCreateCS( db, csName, true, "Failed to createCS." );
   }

   //createCL
   try
   {
      db.execUpdate( "create collection " + csName + "." + clName );
   }
   catch( e )
   {
      if( e != -22 || !ignoreExisted )
      {
         println( message );
         throw e;
      }
   }

   //getCL
   try
   {
      return eval( 'db.' + csName + '.getCL("' + clName + '")' );
   }
   catch( e )
   {
      println( "Failed to getCL." );
      throw e;
   }

}

function dropCL ( csName, clName, ignoreNotExist, message )
{
   println( "\n---Begin to drop CL." );

   if( message == undefined ) { message = ""; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }

   try
   {
      db.execUpdate( "drop collection " + csName + "." + clName );
   }
   catch( e )
   {
      if( ( e !== -34 && ignoreNotExist ) || ( e === -23 && ignoreNotExist ) )
      {
         //continue
      }
      else
      {
         println( message );
         throw e;
      }
   }
}