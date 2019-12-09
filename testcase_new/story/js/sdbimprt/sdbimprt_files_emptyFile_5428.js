/************************************************************************
*@Description:   seqDB-5428:导入的CVS/JSON文件为空
*@Author:        2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5428";
      var cl = readyCL( csName, clName );

      var imprtFile1 = tmpFileDir + "5428.json";
      var imprtFile2 = tmpFileDir + "5428.csv";
      readyData( imprtFile1, imprtFile2 );
      importData( csName, clName, imprtFile1, imprtFile2 );

      checkCLData( cl );
      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function readyData ( imprtFile1, imprtFile2 )
{
   println( "\n---Begin to ready data." );

   var file = fileInit( imprtFile1 );
   var fileInfo = cmd.run( "cat " + imprtFile1 );
   println( imprtFile1 + "\n" + fileInfo );
   file.close();

   var file = fileInit( imprtFile2 );
   var fileInfo = cmd.run( "cat " + imprtFile2 );
   println( imprtFile2 + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile1, imprtFile2 )
{
   println( "\n---Begin to import data and check exec result." );

   //json file is empty
   var imprtOption1 = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + imprtFile1;
   println( imprtOption1 );
   var rc1 = cmd.run( imprtOption1 );
   println( rc1 );
   var rcObj = rc1.split( "\n" );
   var expParseRecords = "parsed records: 0";
   var expImportedRecords = "imported records: 0";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expParseRecords + ", " + expImportedRecords + "]",
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }

   //csv file is empty
   var imprtOption2 = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a'
      + ' --file ' + imprtFile2;
   println( imprtOption2 );
   var rc2 = cmd.run( imprtOption2 );
   println( rc2 );
   var rcObj = rc2.split( "\n" );
   var expParseRecords = "parsed records: 0";
   var expImportedRecords = "imported records: 0";
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 0;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[cnt:" + expCnt + "]",
         "[cnt:" + actCnt + "]" );
   }

}