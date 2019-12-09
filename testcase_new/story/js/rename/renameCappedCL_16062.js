/* *****************************************************************************
@discretion: rename capped cl, than insert datas
@author��2018-10-15 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var csName = CHANGEDPREFIX + "_renamecs16062";
      var clName = CHANGEDPREFIX + "_renamecl16062";
      var newCLName = CHANGEDPREFIX + "_newcl16062";
      commDropCS( db, csName, true, "drop CS in the beginning" );

      println( "---begin to create cappedCS and cappedCL" );
      var options = { Capped: true };
      var dbcs = commCreateCS( db, csName, false, "create cappedCS", options );
      var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
      commCreateCLByOption( db, csName, clName, optionObj, false, false, "create cappedCL" );

      println( "--begin to rename cappedCL and check result." );
      dbcs.renameCL( clName, newCLName );
      checkRenameCLResult( csName, clName, newCLName );

      println( "---begin to insert data and check result." );
      var recordNums = 2000;
      var dbcl = dbcs.getCL( newCLName );
      insertData( dbcl, recordNums );
      checkDatas( csName, newCLName, recordNums );

      commDropCS( db, csName, true, "drop CS in the ending" );
   }
   catch( e )
   {
      throw e;
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
