/************************************************************************
*@Description:    seqDB-5489:导入数据时指定的数据类型为binary，实际数据支持转换的数据类型
                    string  binary
                  seqDB-5490
*@Author:   2016-8-1  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5489";
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
      + ' --type csv --fields "num int,type string,v1 binary,v2 binary"'
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
   var actFailedNum = cmd.run( "cat " + rec ).split( "\n" ).length - 1;
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 5 } }, { v2: { $type: 1, $et: 5 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 6;
   var expRecs = '[{"num":8,"type":"null","v1":{"$binary":"null","$type":"0"},"v2":{"$binary":"null","$type":"0"}},{"num":9,"type":"oid","v1":{"$binary":"5791b549b1f90a1171000016","$type":"0"},"v2":{"$binary":"abcdef01230123456789cdef","$type":"0"}},{"num":12,"type":"binary","v1":{"$binary":"aGVsbG8gd29ybGQ=","$type":"0"},"v2":{"$binary":"adc=","$type":"0"}},{"num":37,"type":"nullStr","v1":{"$binary":"null","$type":"0"},"v2":{"$binary":"null","$type":"0"}},{"num":38,"type":"oidStr","v1":{"$binary":"5791b549b1f90a1171000016","$type":"0"},"v2":{"$binary":"abcdef01230123456789cdef","$type":"0"}},{"num":41,"type":"binaryStr","v1":{"$binary":"aGVsbG8gd29ybGQ=","$type":"0"},"v2":{"$binary":"adc=","$type":"0"}}]';
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