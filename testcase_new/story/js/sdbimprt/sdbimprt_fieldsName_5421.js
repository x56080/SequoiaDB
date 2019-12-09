/************************************************************************
*@Description:    seqDB-5421:指定首行作为字段名，首行为空
*@Author:        2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5421";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5421.csv";
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
   file.write( "\n1,2" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --fields a,b'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "ERROR: the headerline is empty";
   var expParseRecords = "parsed records: 0";
   var expImportedRecords = "imported records: 0";
   var actError = rcObj[0];
   var actParseRecords = rcObj[1];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expParseRecords + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actParseRecords + ", " + actImportedRecords + "]" );
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