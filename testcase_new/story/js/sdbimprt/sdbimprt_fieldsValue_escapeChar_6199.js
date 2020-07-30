/************************************************************************
*@Description:    seqDB-6199 : csv文件最后一个字段为字符串，且有带字符分隔符
*@Author:            2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6199";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "6199.csv";
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
   file.write( "'_id','number','address','commit'\n1,9999999999,'http:sequoiadb.com',true\n2,9999999999.9999999999,'hepaticochlecystostcholecystntenterostomy','false'\n3,0.006574839201,pOiUyTmghe,'null'\n4,+6574839201,-6574839201,6574839201QEC.M\n5,6E+9,2014-4-9,_single109-100.jpg\n6,-6E+9,0xFFFF,006574839201\n7,'914321484','9999999999.9999999999','006574839201'" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --linepriority false'
      + ' -a "\'"'
      + ' --file ' + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "parsed records: 7";
   var expImportedRecords = "imported records: 7";
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "_id": 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 7;
   var expRecs = '[{"number":9999999999,"address":"http:sequoiadb.com","commit":true},{"number":{"$decimal":"9999999999.9999999999"},"address":"hepaticochlecystostcholecystntenterostomy","commit":"false"},{"number":0.006574839201,"address":"pOiUyTmghe","commit":"null"},{"number":6574839201,"address":-6574839201,"commit":"6574839201QEC.M"},{"number":6000000000,"address":"2014-4-9","commit":"_single109-100.jpg"},{"number":-6000000000,"address":"0xFFFF","commit":6574839201},{"number":"914321484","address":"9999999999.9999999999","commit":"006574839201"}]';
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