/*******************************************************************************
*@Description:   seqDB-15772:fields֧��ʱ����ʽ
*@Author:        2018-9-10  wangkexin
********************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_15772";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "15772.csv";
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
   file.write( '_id,time1,time2,time3\n1,"2014-01-01","2001/01/01","1990-01-01"\n2,"2014-01-01Z","2001/01/01Z","1990-01-01Z"\n3,"2014-01-01+0200","2001/01/01+0200","1990-01-01+0200"\n4,"2014-01-01 helloworld","2001/01/01 helloworld","1990-01-01 helloworld"' );
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
      + ' --type csv '
      + ' --fields=\'_id int, time1 timestamp("YYYY-MM-DD"), time2 timestamp("YYYY/MM/DDZ"), time3 timestamp("YYYY-MM-DD+0600")\' '
      + ' --headerline true '
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 4";
   var expParseFailure = "parse failure: 0";
   var expImportedRecords = "imported records: 4";
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "_id": 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 4;
   var expRecs = '[{"time1":{"$timestamp":"2014-01-01-00.00.00.000000"},"time2":{"$timestamp":"2001-01-01-00.00.00.000000"},"time3":{"$timestamp":"1990-01-01-02.00.00.000000"}},'
      + '{"time1":{"$timestamp":"2014-01-01-00.00.00.000000"},"time2":{"$timestamp":"2001-01-01-08.00.00.000000"},"time3":{"$timestamp":"1990-01-01-08.00.00.000000"}},'
      + '{"time1":{"$timestamp":"2014-01-01-00.00.00.000000"},"time2":{"$timestamp":"2001-01-01-06.00.00.000000"},"time3":{"$timestamp":"1990-01-01-06.00.00.000000"}},'
      + '{"time1":{"$timestamp":"2014-01-01-00.00.00.000000"},"time2":{"$timestamp":"2001-01-01-00.00.00.000000"},"time3":{"$timestamp":"1990-01-01-02.00.00.000000"}}]';
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
