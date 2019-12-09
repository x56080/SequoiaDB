/************************************************************************
*@Description:  seqDB-8537:把decimal类型的数据导出成json格式的文件
*@Author:           2016-8-3  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_8537";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "8537.csv";
      var exprtFile = tmpFileDir + "sdbexprt_8537.json";

      readyData( imprtFile );
      importData( csName, clName, imprtFile );
      checkCLData( cl );

      exprtData( csName, clName, exprtFile );
      checkExprtFile( exprtFile );

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
   file.write( "1,9223372036854775808\n"
      + "2,-9223372036854775809\n"
      + "3, 92233720368547758079223372036854775807\n"
      + "4,-92233720368547758079223372036854775807\n"
      + "5, 92233720368547758089223372036854775808\n"
      + "6,-92233720368547758079223372036854775808\n"
      + "7,0.922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "a int,b decimal"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcObj = rc.split( "\n" );

   //check import results
   var expParseRecords = "parsed records: 7";
   var expParseFailure = "parse failure: 0";
   var expImportedRecords = "imported records: 7";
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
}

function checkCLData ( cl )
{
   println( "\n---Begin to check cl data." );

   var rc = cl.find( { b: { $type: 1, $et: 100 } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 7;
   var expRecs = '[{"a":1,"b":{"$decimal":"9223372036854775808"}},{"a":2,"b":{"$decimal":"-9223372036854775809"}},{"a":3,"b":{"$decimal":"92233720368547758079223372036854775807"}},{"a":4,"b":{"$decimal":"-92233720368547758079223372036854775807"}},{"a":5,"b":{"$decimal":"92233720368547758089223372036854775808"}},{"a":6,"b":{"$decimal":"-92233720368547758079223372036854775808"}},{"a":7,"b":{"$decimal":"0.922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368"}}]';
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

function exprtData ( csName, clName, exprtFile )
{
   println( "\n---Begin to export data." );

   //remove export file
   cmd.run( "rm -rf " + exprtFile );

   //export operation
   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json --fields "a,b"'
      + ' --sort "{a:1}" --file ' + exprtFile;
   println( exportOption );
   var rc = cmd.run( exportOption );
   println( rc );

   //cat exprt file
   var fileInfo = cmd.run( "cat " + exprtFile );
   println( exprtFile + "\n" + fileInfo );
}

function checkExprtFile ( exprtFile )
{
   println( "\n---Begin to check export file data." );

   var rcObj = cmd.run( "cat " + exprtFile ).split( "\n" );
   var actRC = JSON.stringify( rcObj );
   var expRC = '["{ \\"a\\": 1, \\"b\\": { \\"$decimal\\": \\"9223372036854775808\\" } }","{ \\"a\\": 2, \\"b\\": { \\"$decimal\\": \\"-9223372036854775809\\" } }","{ \\"a\\": 3, \\"b\\": { \\"$decimal\\": \\"92233720368547758079223372036854775807\\" } }","{ \\"a\\": 4, \\"b\\": { \\"$decimal\\": \\"-92233720368547758079223372036854775807\\" } }","{ \\"a\\": 5, \\"b\\": { \\"$decimal\\": \\"92233720368547758089223372036854775808\\" } }","{ \\"a\\": 6, \\"b\\": { \\"$decimal\\": \\"-92233720368547758079223372036854775808\\" } }","{ \\"a\\": 7, \\"b\\": { \\"$decimal\\": \\"0.922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368547758089223372036854775808922337203685477580892233720368\\" } }",""]';

   if( actRC !== expRC )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[exprtFile data:" + expRC + "]",
         "[exprtFile data:" + actRC + "]" );
   }

}