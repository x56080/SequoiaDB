/* *****************************************************************************
@discretion: rename cl,index in cl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16056";
      var newCLName = CHANGEDPREFIX + "_newcl16056";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var dbcl = commCreateCL( db, COMMCSNAME, clName, 0 );

      var recordNums = 2000;
      var indexName = "testindex";
      insertData( dbcl, recordNums );
      dbcl.createIndex( indexName, { no: 1 } );

      println( "---begin to rename cl" );
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

      println( "---begin to check result" );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkFindResult( COMMCSNAME, newCLName, indexName, recordNums );

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function checkFindResult ( csName, newCLName, expIndexName, expRecordNums )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( newCLName );
      var count = dbcl.count();
      if( count != expRecordNums )
      {
         throw buildException( "check record count", null, "check the new cl record nums",
            expRecordNums, count );
      }

      //check index
      var indexName = dbcl.find( { no: 1 } ).explain().current().toObj().IndexName;
      if( indexName !== expIndexName )
      {
         throw buildException( "check find by index", null, "check find by index",
            indexName, expIndexName );
      }

   }
   catch( e )
   {
      throw buildException( "checkRenameCLResult", e )
   }
}
