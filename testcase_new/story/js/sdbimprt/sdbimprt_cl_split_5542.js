/************************************************************************
*@Description:    seqDB-5542:导入数据成功后对数据做基本操作，并切分
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }

      var groupsArray = getDataGroupsName();
      if( groupsArray.length < 2 )
      {
         println( "Groups least 2." );
         return;
      }

      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5542";
      var optObj = { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupsArray[0], ReplSize: 0 };
      var cl = readyCL( csName, clName, optObj );

      var imprtFile = tmpFileDir + "5542.csv";
      readyData( imprtFile );
      importData( csName, clName, imprtFile );
      splitOper( cl, groupsArray );

      checkCLData( csName, clName, groupsArray );
      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function splitOper ( cl, groupsArray )
{
   println( "\n---Begin to split." );

   cl.split( groupsArray[0], groupsArray[1], { a: 50 }, { a: 100 } );
}

function readyData ( imprtFile )
{
   println( "\n---Begin to ready data." );

   var file = fileInit( imprtFile );
   for( i = 0; i < 100; i++ )
   {
      file.write( i + "\n" );
   }
   /*
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   */
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "a int"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcObj = rc.split( "\n" );

   var expParseRecords = "parsed records: 100";
   var expImportedRecords = "imported records: 100";
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

function checkCLData ( csName, clName, groupsArray )
{
   println( "\n---Begin to check cl data." );

   var cl = db.getCS( csName ).getCL( clName );
   var rc = cl.find( {}, { _id: { $include: 0 } } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 100;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[cnt:" + expCnt + "]",
         "[cnt:" + actCnt + "]" );
   }

   println( "   check source group data." );
   var srcAddr = db.getRG( groupsArray[0] ).getMaster().toString();
   var actCnt1 = new Sdb( srcAddr ).getCS( csName ).getCL( clName ).find( { a: { $lt: 50 } } ).count();
   var expCnt1 = 50;
   if( Number( actCnt1 ) !== expCnt1 )
   {
      throw buildException( "check source group data", null, "[find.count]",
         "[srcDataCnt:" + expCnt1 + "]",
         "[srcDataCnt:" + actCnt1 + "]" );
   }

   println( "   check target group data." );
   var trgAddr = db.getRG( groupsArray[1] ).getMaster().toString();
   var actCnt2 = new Sdb( trgAddr ).getCS( csName ).getCL( clName ).find( { a: { $gte: 50 } } ).count();
   var expCnt2 = 50;
   if( Number( actCnt2 ) !== expCnt2 )
   {
      throw buildException( "check target group data", null, "[find.count]",
         "[trgDataCnt:" + expCnt2 + "]",
         "[trgDataCnt:" + actCnt2 + "]" );
   }
}