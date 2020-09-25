/************************************************************************
*@Description:    seqDB-5476:导入数据时指定的数据类型为date，实际数据为支持转换的数据类型
                    int 	long 	string 	date 	timestamp
                  seqDB-5477/seqDB-5479
*@Author:   2016-7-29  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5476";
      var cl = readyCL( csName, clName );

      var imprtFile = testCaseDir + "dataFile/allDataType.csv";
      importData( csName, clName, imprtFile );

      checkCLData( cl );
      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //cat import file
   //var fileInfo = cmd.run( "cat "+ imprtFile );
   //println( imprtFile +"\n" + fileInfo +"\n" );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string,v1 date,v2 date"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 6";
   var expParseFailure = "parse failure: 30";
   var expImportedRecords = "imported records: 6";
   var actParseRecords = rcObj[0];
   var actParseFailure = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]",
         "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }

   //check failed records
   var rec = cmd.run( "ls " + tmpRec ).split( "\n" )[0];
   var actFailedNum = cmd.run( "cat -v " + rec ).split( "\n" ).length - 1;
   println( rec + "\nrecords number: " + actFailedNum );
   var expFailedNum = 30;
   if( expFailedNum !== actFailedNum )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[failedRecs:" + expFailedNum + "]",
         "[failedRecs:" + actFailedNum + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{
   println( "\n---Begin to check cl data." );

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 9 } }, { v2: { $type: 1, $et: 9 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 5;
   var expRecs = '[{"num":10,"type":"date","v1":{"$date":"1900-01-01"},"v2":{"$date":"9999-12-31"}},{"num":11,"type":"timestamp","v1":{"$date":"1902-01-01"},"v2":{"$date":"2037-12-31"}},{"num":39,"type":"dateStr","v1":{"$date":"1900-01-01"},"v2":{"$date":"9999-12-31"}},{"num":40,"type":"timestampStr","v1":{"$date":"1902-01-01"},"v2":{"$date":"2037-12-31"}},{"num":55,"type":"timestamp","v1":{"$date":"1901-12-31"},"v2":{"$date":"2038-01-01"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[cnt:" + expCnt + ", recs:" + expRecs + "]",
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
   //println( "cl records: "+ actRecs );

}