/************************************************************************
*@Description:  seqDB-6109:导入浮点型数据后再执行sdbexprt导出
*@Author:   2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6109";
      var cl = readyCL( csName, clName );

      var imprtFile = testCaseDir + "dataFile/autoToJudge.csv";
      var exprtFile = tmpFileDir + "sdbexprt_6109.csv";

      importData( csName, clName, imprtFile );
      exprtData( csName, clName, exprtFile );

      checkExprtFile( exprtFile );
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

   //cat import file
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo + "\n" );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,oriV1,type string,srcV2"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 18";
   var expParseFailure = "parse failure: 0";
   var expImportedRecords = "imported records: 18";
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

function exprtData ( csName, clName, exprtFile )
{
   println( "\n---Begin to export data." );

   //remove export file
   cmd.run( "rm -rf " + exprtFile );

   //export operation
   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num,srcV2"'
      + ' --sort "{num:1}" --file ' + exprtFile;
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
   var expRC = '["num,srcV2","1,123","2,123","3,123","4,-123","5,123","6,-123","7,2147483648","8,123.1","9,0.123","10,9223372036854775808","11,true","12,false","13,\\"123\\"","14,\\"123a\\"","15,\\"true\\"","16,\\"false\\"","17,\\"null\\"","18,null",""]';

   if( actRC !== expRC )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[exprtFile data:" + expRC + "]",
         "[exprtFile data:" + actRC + "]" );
   }


}