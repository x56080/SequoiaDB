/************************************************************************
*@Description:   seqDB-5444:指定并发数为1000（最大并发数）
*@Author:        2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5444";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5444.csv";
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
   for( i = 0; i < 2000; i++ )
   {
      file.write( String( i + ",test_" + i + "\n" ) );
   }
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a,b'
      + ' --insertnum 2 --jobs 200'  //1000
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   sleep( 1500 );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 2000";
   var expImportedRecords = "imported records: 2000";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expParseRecords + ", " + expImportedRecords + "]",
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( cl )
{
   println( "\n---Begin to check cl data." );

   var expCnt = 2000;
   var expMinRecs = '{"a":0,"b":"test_0"}';
   var expMaxRecs = '{"a":1999,"b":"test_1999"}';
   var actCnt = Number( cl.count() );
   var actMinRecs = JSON.stringify( cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ).limit( 1 ).current().toObj() );
   var actMaxRecs = JSON.stringify( cl.find( {}, { _id: { $include: 0 } } ).sort( { a: -1 } ).limit( 1 ).current().toObj() );
   if( expCnt !== actCnt || expMinRecs !== actMinRecs || expMaxRecs !== actMaxRecs )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[cnt:" + expCnt + ", minRecs:" + expMinRecs + ", maxRecs" + expMaxRecs + "]",
         "[cnt:" + actCnt + ", minRecs:" + actMinRecs + ", maxRecs" + actMaxRecs + "]" );
   }
   println( "cl count: " + actCnt );
   println( "cl minRecs: " + actMinRecs );
   println( "cl maxRecs: " + actMaxRecs );

}