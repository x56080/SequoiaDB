/*******************************************************************************
*@Description:   seqDB-15755:导入首字符是双引号的key的json格式数据记录
*                seqDB-15756:导入&命令中间插入双引号的key的json格式数据记录
*@Author:        2018-9-6  wangkexin
********************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_15755";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "15755.json";
      readyData( imprtFile );
      importData( csName, clName, imprtFile );

      checkCLData( cl );
      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function readyData ( imprtFile )
{
   println( "\n---Begin to ready data." );

   var file = fileInit( imprtFile );
   file.write( '{ "_id": 1, "\\\"testa": "hello" }{"_id": 2, "te\\\"stb":"test"}{"_id": 3,"testc":"test"}{"_id": 4, "testd": { "$number\\\"Long": 9223372036854775807 } }{ "_id": 5, "teste": { "\\\"$numberLong": 9223372036854775807 } }{ "_id": 6, "testf": { "$numberLong": 9223372036854775807 } }' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json '
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 5";
   var expParseFailure = "parse failure: 1";
   var expImportedRecords = "imported records: 5";
   var actParseRecords = rcObj[0];
   var actParseFailure = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]",
         "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{
   println( "\n---Begin to check cl data." );

   var rc = cl.find( {}, { "_id": { $include: 0 } } ).sort( { "_id": 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 5;
   var expRecs = '[{"\\\"testa":"hello"},{"te\\\"stb":"test"},{"testc":"test"},{"teste":{"\\\"$numberLong":{"$numberLong":"9223372036854775807"}}},{"testf":{"$numberLong":"9223372036854775807"}}]';
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
