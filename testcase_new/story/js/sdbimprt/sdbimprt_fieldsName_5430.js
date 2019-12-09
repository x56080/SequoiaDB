/************************************************************************
*@Description:   seqDB-5430:字段名重复（包括首行字段名重复、指定的字段名重复）
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5430";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5430.csv";
      readyData( imprtFile );
      importData( csName, clName, imprtFile );

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
   file.write( "a,a,btest" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   //backup sdbimport.log and generate new sdbimport.log
   var rt = cmd.run( 'find ./ -name "sdbimport.log"' );
   if( rt !== '' )  //file is exist
   {
      var time = cmd.run( 'date "+%Y-%m-%d-%H:%M:%S"' );
      cmd.run( 'mv ./sdbimport.log sdbimport.log_' + time );
   }

   //---------------------scene1-------------------------
   println( '-----------scene1--Begin to import, headerline exist duplicate field name: a-----------\n' );
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
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

   //check sdbimport.log
   var logInfo = cmd.run( 'find ./ -name "sdbimport.log" |xargs grep "duplicate field name: a"' ).split( "\n" );
   println( "sdbimport.log: \n" + logInfo[0] );
   var expV = 2;
   var actV = logInfo.length;
   if( expV !== actV )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expV + "]",
         "[" + actV + "]" );
   }

   //---------------------scene2-------------------------
   println( '\n-----------scene2--Begin to import, fields exist duplicate field name: b------------\n' );
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "b,b,ctest"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
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

   //check sdbimport.log
   var logInfo = cmd.run( 'find ./ -name "sdbimport.log" |xargs grep "duplicate field name: b"' ).split( "\n" );
   println( "sdbimport.log: \n" + logInfo[0] );
   var expV = 2;
   var actV = logInfo.length;
   if( expV !== actV )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expV + "]",
         "[" + actV + "]" );
   }
}