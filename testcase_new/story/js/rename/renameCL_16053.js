/* *****************************************************************************
@discretion: rename cl, the new cl name is the same as the old cl name
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      //create cl 
      var clName = CHANGEDPREFIX + "_renamecl16053";
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0 }, true, true );

      //rename cl   
      try
      {
         db.getCS( COMMCSNAME ).renameCL( clName, clName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e != -22 )
         {
            throw buildException( "rename cl the same as old name:", e );
         }
      }

      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }

}

