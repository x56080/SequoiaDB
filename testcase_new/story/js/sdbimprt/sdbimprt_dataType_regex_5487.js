/************************************************************************
*@Description:    seqDB-5487:导入数据时指定的数据类型为regex，实际数据为支持转换的数据类型string，取值为非法的正则表达式
*@Author:            2016-8-1  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5487";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5487.csv";
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
   file.write( '1,"^[0-9]"' + "\n"
      + '2,"/^[0-9]/"' + "\n"
      + '3,"/^[0-9]/i"' + "\n"
      + '4,"/^[0-9]/m"' + "\n"
      + '5,"/^[0-9]/x"' + "\n"
      + '6,"/^[0-9]/test"' + "\n"
      + '7,"/^[0-9]"' );
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

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,v1 regex"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 6";
   var expParseFailure = "parse failure: 1";
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
   var expFailedNum = 1;
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

   var rc = cl.find( { v1: { $type: 1, $et: 11 } }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 6;
   var expRecs = '[{"num":1,"v1":{"$regex":"^[0-9]","$options":""}},{"num":2,"v1":{"$regex":"^[0-9]","$options":""}},{"num":3,"v1":{"$regex":"^[0-9]","$options":"i"}},{"num":4,"v1":{"$regex":"^[0-9]","$options":"m"}},{"num":5,"v1":{"$regex":"^[0-9]","$options":"x"}},{"num":6,"v1":{"$regex":"^[0-9]","$options":"test"}}]';
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