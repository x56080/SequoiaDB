/* *****************************************************************************
@discretion: rename cl,than insert/update/find/delete datas, putLob/getLob/deleteLob
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16068";
      var newCLName = CHANGEDPREFIX + "_newcl16068";
      var fileName = CHANGEDPREFIX + "_lobtest16068.file";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      commCreateCL( db, COMMCSNAME, clName, 0 );

      println( "---begin to rename cl" );
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

      println( "---begin to insert and put lob" );
      var dbcl = db.getCS( COMMCSNAME ).getCL( newCLName );
      var recordNums = 1000;
      insertData( dbcl, recordNums );
      var srcMd5 = createFile( fileName );
      var lobIdArr = putLobs( dbcl, fileName );

      println( "---begin to check rename cl ,insert datas and lob datas" );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkDatas( COMMCSNAME, newCLName, recordNums, srcMd5, lobIdArr );

      updateDataAndCheckResult( dbcl, recordNums );
      removeDataAndCheckResult( dbcl, recordNums );
      truncateLobAndCheckResult( dbcl, lobIdArr );

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

function updateDataAndCheckResult ( dbcl, expRecordNums )
{
   try
   {
      println( "---begin to update data and checkResult" );
      dbcl.update( { $set: { "user": "testupdate" } } );
      var count = dbcl.count( { "user": "testupdate" } );
      if( count != expRecordNums )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            expRecordNums, count );
      }
   }
   catch( e )
   {
      throw buildException( "updateDatas", e )
   }
}

function removeDataAndCheckResult ( dbcl, recordNums )
{
   try
   {
      println( "---begin to remove data and checkResult" );
      var removeNums = 500;
      dbcl.remove( { a: { $gte: removeNums } } );
      var removeDataNumsInCL = dbcl.count( { a: { $gte: removeNums } } );
      var expRemoveNumsInCL = 0;
      if( removeDataNumsInCL != expRemoveNumsInCL )
      {
         throw buildException( "check removed datas", null, "check the record nums",
            expRemoveNumsInCL, removeDataNumsInCL );
      }

      var count = dbcl.count( { a: { $lt: removeNums } } );
      var expRecordNums = recordNums - removeNums;
      if( count != expRecordNums )
      {
         throw buildException( "check cl datas", null, "check the cl record nums",
            expRecordNums, count );
      }
   }
   catch( e )
   {
      throw buildException( "removeDatas", e )
   }
}

function truncateLobAndCheckResult ( dbcl, expLobArr )
{
   println( "---begin to truncateLob and checkResult" );
   var lobOid = expLobArr[0];
   var lobSize = 100;
   dbcl.truncateLob( lobOid, lobSize );

   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var lobInfo = rc.current().toObj();
      var actSize = lobInfo["Size"];
      var isNormal = lobInfo["Available"];
      if( actSize !== lobSize )
      {
         throw buildException( "check Available", null, "cl.listLobs()", lobSize, actSize );
      }
      if( isNormal !== true )
      {
         throw buildException( "check Available", null, "cl.listLobs()",
            '"Available":true', '"Available":' + isNormal );
      }
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

function truncateCLAndCheckResult ( dbcl )
{
   try
   {
      dbcl.truncate();

      //check the records nums is 0
      var count = dbcl.count();
      if( count !== 0 )
      {
         throw buildException( "check truncate datas", null, "check the new cl record nums",
            0, expRecordNums );
      }

      //check the lob
      var lobInfos = dbcl.listLobs().next();
      if( lobInfos !== undefined )
      {
         throw buildException( "check truncate lobs", null, "check the new cl lob nums",
            "undefined", lobInfos );
      }
   }
   catch( e )
   {
      throw buildException( "checktruncatecl", e )
   }
}
