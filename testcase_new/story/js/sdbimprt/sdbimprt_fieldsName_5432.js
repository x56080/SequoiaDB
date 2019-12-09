/************************************************************************
*@Description:    seqDB-5432:字段名非法参数校验
*@Author:   2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5432";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5432.csv";
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
      + ' --type csv --headerline true'
      + ' --file ' + imprtFile;
   //------------------scene:1--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'$a'\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );

   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
   var expImportedRecords = "imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:2--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'a.b\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );

   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
   var expImportedRecords = "imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:3--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "''\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );

   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
   var expImportedRecords = "imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:4--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'a\t'\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );

   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expError = "failed to parse fields";
   var expImportedRecords = "imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   file.close();
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