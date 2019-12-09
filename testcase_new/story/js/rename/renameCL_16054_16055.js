/* *****************************************************************************
@discretion: rename cl, test case 16054/16055:
             testcase-16054:the old cl is not exist
             testcase-16055:the new cl is exist
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16054";
      var newCLName = CHANGEDPREFIX + "_newcl16054";
      var clName1 = CHANGEDPREFIX + "_renamecl16055";
      var newCLName = CHANGEDPREFIX + "_newcl16054";
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, clName1, true, true, "clear collection in the beginning" );
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName1, { ReplSize: 0 }, true, true );
      commCreateCL( db, COMMCSNAME, newCLName, 0 );

      //test case-16054: the old cl is not exist
      println( "---begin to test case 16064" );
      try
      {
         db.getCS( COMMCSNAME ).renameCL( clName, newCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e != -23 )
         {
            throw buildException( "rename cl16054:", e );
         }
      }

      //test case-16055: the new cl is exist
      println( "---begin to test case 16065" );
      try
      {
         db.getCS( COMMCSNAME ).renameCL( clName1, newCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e != -22 )
         {
            throw buildException( "rename cl16055:", e );
         }
      }

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }

}

