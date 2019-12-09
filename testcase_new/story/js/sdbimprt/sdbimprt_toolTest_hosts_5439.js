/************************************************************************
*@Description:   seqDB-5439:连接多个coord并发导入数据，指定并发数小于连接指定的coord数
*@Author:        2016-7-14  huangxiaoni
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

      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5439";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5439.csv";
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
   file.write( "a int,b int\n1,1\n2,2\n3,3\n4,4" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   //get coord address
   var coordAddrs = getCoordAdrr();

   var imprtOption = installDir + 'bin/sdbimprt'
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --hosts "' + String( coordAddrs )
      + '" -n 1 -j 2 -v'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 4";
   var expImportedRecords = "imported records: 4";
   var actParseRecords = rcObj[11];
   var actImportedRecords = rcObj[15];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
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

   var expCnt = 4;
   var expRecs = '[{"a":1,"b":1},{"a":2,"b":2},{"a":3,"b":3},{"a":4,"b":4}]';
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

function getCoordAdrr ()
{
   println( "\n---Begin to get coord address." );
   var nodeArray = [];
   var tmpInfo = db.listReplicaGroups().toArray();
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" );
      if( tmpObj.GroupName == "SYSCoord" )
      {
         var tmpGroupObj = tmpObj.Group;
         for( var j = 0; j < tmpGroupObj.length; ++j )
         {
            var tmpNodeObj = tmpGroupObj[j];
            nodeArray.push( tmpNodeObj.HostName + ":" + tmpNodeObj.Service[0].Name );
         }
      }
   }
   println( "-------nodeArray : " + nodeArray );
   return nodeArray;
}