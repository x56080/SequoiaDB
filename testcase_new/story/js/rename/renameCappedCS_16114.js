/* *****************************************************************************
@discretion: rename capped cs, than create cappedcl and delete cappedcl
@author��2018-10-16 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var csName = CHANGEDPREFIX + "_renamecs16114";
      var newCSName = CHANGEDPREFIX + "_newcs16114";
      var cLName = CHANGEDPREFIX + "_renamecl16114";
      commDropCS( db, csName, true, "drop CS in the beginning" );
      commDropCS( db, newCSName, true, "drop CS in the beginning" );

      println( "---begin to create cappedCS and cappedCL" );
      var options = { Capped: true };
      var dbcs = commCreateCS( db, csName, false, "create cappedCS", options );
      var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
      var dbcl = commCreateCLByOption( db, csName, cLName, optionObj, false, false, "create cappedCL" );

      println( "---begin to rename cappedCS and check result." );
      var recordNums = 2000;
      insertData( dbcl, recordNums );
      db.renameCS( csName, newCSName );
      checkRenameCSResult( csName, newCSName, 1 );
      checkDatas( newCSName, cLName, recordNums );

      println( "---begin to check result." );
      var dbnewCL = db.getCS( newCSName ).getCL( cLName );
      checkDatas( newCSName, cLName, recordNums );
      dropCappedCLAndCheckResult( newCSName, cLName );
      createCappedCLAndCheckResult( newCSName, cLName );

      commDropCS( db, newCSName, true, "drop CS in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function dropCappedCLAndCheckResult ( cappedCSName, cappedCLName )
{
   try
   {

      var dbcs = db.getCS( cappedCSName );
      dbcs.dropCL( cappedCLName );

      //check the cl is not exist
      try
      {
         dbcs.getCL( cappedCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e !== -23 )
         {
            throw buildException( "check dropCappedCL:", e );
         }
      }
   }
   catch( e )
   {
      throw buildException( "dropCappedCLAndCheckResult", e )
   }
}

function createCappedCLAndCheckResult ( cappedCSName, cappedCLName )
{
   try
   {
      //create cappedcl,the cl name is the same as the deleted name    
      var dbcs = db.getCS( cappedCSName );
      var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
      var dbcl = commCreateCLByOption( db, cappedCSName, cappedCLName, optionObj, false, false, "create cappedCL" );

      var count = dbcl.count();
      if( Number( count ) !== 0 )
      {
         throw buildException( "check new cappedCL data nums:", e );
      }
   }
   catch( e )
   {
      throw buildException( "create new cappedCL", e )
   }
}


function checkDatas ( csName, newCLName, expRecordNums )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( newCLName );
      var count = dbcl.count( { "$and": [{ a: { "$gte": 0 } }, { a: { "$lt": expRecordNums } }] } );
      if( count != expRecordNums )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            expRecordNums, count );
      }
   }
   catch( e )
   {
      throw buildException( "checkDatas", e )
   }
}
