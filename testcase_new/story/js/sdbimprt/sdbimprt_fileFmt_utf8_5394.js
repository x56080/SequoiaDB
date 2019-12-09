/************************************************************************
*@Description:  seqDB-5394:导入JSON文件数据，文件编码为不带BOM的UTF-8格式
*@Author:            2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5394";
      var cl = readyCL( csName, clName );

      var imprtFile = testCaseDir + "dataFile/UTF-8_NO-BOM.json";
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

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 2";
   var expImportedRecords = "imported records: 2";
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
   println( "\n---Begin to check cl data after the sdbimprt operation." );

   var actCnt = 2;
   var expCnt = Number( cl.count() );
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[cl count]",
         "[cnt:" + expCnt + "]",
         "[cnt:" + actCnt + "]" );
   }
}