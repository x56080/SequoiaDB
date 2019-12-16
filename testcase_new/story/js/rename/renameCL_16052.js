/* *****************************************************************************
@discretion: rename cl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16052";
      var newCLName = CHANGEDPREFIX + "_newcl16052";
      var fileName = CHANGEDPREFIX + "_lobtest16052.file";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var dbcl = commCreateCL( db, COMMCSNAME, clName );

      //insert records and lob 
      var recordNums = 2000;
      insertData( dbcl, recordNums );

      var srcMd5 = createFile( fileName );
      var lobIdArr = putLobs( dbcl, fileName );

      println( "---begin to rename cl" );
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

      println( "---begin to check result" );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkDatas( COMMCSNAME, newCLName, recordNums, srcMd5, lobIdArr );

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf *" + fileName );
   }
}

function checkDatas ( csName, newCLName, expRecordNums, srcMd5, expLobArr )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( newCLName );
      var count = dbcl.count();
      if( count != expRecordNums )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            expRecordNums, count );
      }

      //check the lob
      checkLob( dbcl, expLobArr, srcMd5 );
   }
   catch( e )
   {
      throw buildException( "checkDatas", e )
   }
}
