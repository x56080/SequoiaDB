/************************************
*@Description:capped cl, repeate insert and pop, direciton set -1
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11796
**************************************/
function main ()
{
   var clName = COMMCAPPEDCLNAME + "_11796";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   var repeatedTimes = 10;
   var minLength = 0;
   var maxLength = 2048;
   repeatedInsertAndPopLastRecord( dbcl, repeatedTimes, minLength, maxLength );
   println( "--end insert and check data" );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function repeatedInsertAndPopLastRecord ( dbcl, repeatedTimes, minLength, maxLength )
{
   try
   {
      var rd = new commDataGenerator();
      var recordNum = 100;
      var recs = rd.getRecords( recordNum, ["int", "string", "bool", "date",
         "binary", "regex", "null"], ['a'] );
      insertDatas( dbcl, recs );

      checkRecords( dbcl, null, null, null, null, null, recs );

      //get the first logicalID
      var firstRecord = dbcl.find().sort( { _id: -1 } ).limit( 1 );
      while( firstRecord.next() )
      {
         var firstLogicalID = firstRecord.current().toObj()._id;
      }

      //repeat pop and insert,check LogicalID is the same
      for( var i = 0; i < repeatedTimes; i++ )
      {
         var record = dbcl.find().sort( { _id: -1 } ).limit( 1 );
         while( record.next() )
         {
            //check logical ID
            var logicalID = record.current().toObj()._id
            if( logicalID !== firstLogicalID )
            {
               println( "actual logicalID: " + logicalID + "\nexpect logicalID: " + firstLogicalID );
               throw "LOGICAL_ID_CHECK_ERROR";
            }

            //check record
            var expectArr = [];
            expectArr.push( recs[recordNum - 1] );
            checkRecords( dbcl, { _id: logicalID }, null, { _id: -1 }, null, null, expectArr );

         }
         dbcl.pop( { LogicalID: logicalID, Direction: -1 } );
         recs.pop();

         var doc = new StringBuffer();
         var range = maxLength - minLength;
         var stringLength = minLength + parseInt( Math.random() * range );
         doc.append( stringLength, "a" );
         var strings = doc.toString();
         var insertRecord = { a: strings };
         dbcl.insert( insertRecord );
         recs.push( insertRecord );
      }
   } catch( e )
   {
      throw buildException( "repeatedInsertAndPopLastRecord", e, null, null, e );

   }
}

main();



