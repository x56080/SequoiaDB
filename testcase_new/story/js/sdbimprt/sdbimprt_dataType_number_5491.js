/************************************************************************
*@Description:   seqDB-5491:导入数据时指定的数据类型为number，实际数据为支持转换的数据类型
                    int 	long 	double 	decimal 	bool 	string 	null
                 seqDB-5492
*@Author:   2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5491";
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
      + ' --type csv --fields "num int,type string,v1 number,v2 number"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 19";
   var expParseFailure = "parse failure: 17";
   var expImportedRecords = "imported records: 19";
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
   var expFailedNum = 17;
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 19;
   var expRecs = '[{"num":1,"type":"int","v1":-2147483648,"v2":2147483647},{"num":2,"type":"long","v1":{"$numberLong":"-9223372036854775808"},"v2":{"$numberLong":"9223372036854775807"}},{"num":3,"type":"double","v1":-1.7e+308,"v2":1.7e+308},{"num":4,"type":"decimal","v1":{"$decimal":"-9223372036854775809.001"},"v2":{"$decimal":"9223372036854775808.001"}},{"num":5,"type":"number","v1":-2147483649,"v2":2147483648},{"num":6,"type":"bool","v1":1,"v2":0},{"num":8,"type":"null","v1":null,"v2":null},{"num":21,"type":"dateMS","v1":-377705145943,"v2":253402271999},{"num":22,"type":"timestampMS","v1":-2147414400000,"v2":2147443199000},{"num":31,"type":"intStr","v1":-2147483648,"v2":2147483647},{"num":32,"type":"longStr","v1":{"$numberLong":"-9223372036854775808"},"v2":{"$numberLong":"9223372036854775807"}},{"num":33,"type":"doubleStr","v1":-1.7e+308,"v2":1.7e+308},{"num":34,"type":"decimalStr","v1":{"$decimal":"-9223372036854775809.001"},"v2":{"$decimal":"9223372036854775808.001"}},{"num":35,"type":"numberStr","v1":-2147483649,"v2":2147483648},{"num":50,"type":"int","v1":-2147483649,"v2":2147483648},{"num":51,"type":"long","v1":{"$decimal":"-9223372036854775809"},"v2":{"$decimal":"9223372036854775808"}},{"num":52,"type":"double","v1":{"$decimal":"-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},"v2":{"$decimal":"180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"num":57,"type":"dateMSStr","v1":-377705145944,"v2":253402272000},{"num":58,"type":"timestampMSStr","v1":-2147414400001,"v2":2147443199001}]';
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