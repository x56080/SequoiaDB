/************************************************************************
*@Description:    seqDB-5436:指定errorstop为false，即导入失败时继续导入
*@Author:        2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5436" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5436.csv";
      readyData( imprtFile );
      importData( csName, clName, imprtFile );
   	
      checkCLData( cl );
      cleanCL( csName, clName );
   }
      catch(e)
   {
   	throw e;
   }
}

function readyData( imprtFile )
{
   println("\n---Begin to ready data.");
   
   var file = fileInit( imprtFile );
   file.write( "at int,bt date\n1,2016-1-1\n2,2\n3,2016-1-2\n4,2016-0-0\n5,2016-1-3" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   //remove rec file
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf "+ tmpRec );
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --headerline true --errorstop false -n 1'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 2";
   var expParseFailure    = "parse failure: 1";
   var expImportedRecords = "imported records: 2";
   var actParseRecords    = rcObj[0];
   var actParseFailure    = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseRecords !== actParseRecords 
    || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expParseFailure +", "+ expImportedRecords +"]", 
                        "["+ actParseRecords +", "+ actParseFailure +", "+ actImportedRecords +"]" );
   }
   
   var rec = cmd.run( "ls "+ tmpRec ).split("\n")[0];
   var failedRecs = cmd.run( "cat "+ rec ).split("\n")[0];
   println( rec +"\n"+ failedRecs );
   var expRecRecs = '2,2';
   var actRecRecs = failedRecs;
   if( expRecRecs !== actRecRecs )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[failedRecs:"+ expRecRecs +"]", 
                        "[failedRecs:"+ actRecRecs +"]" );
   }
   
   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
   
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({},{_id:{$include:0}}).sort({a:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 2;
   var expRecs = '[{"at":1,"bt":{"$date":"2016-01-01"}},{"at":3,"bt":{"$date":"2016-01-02"}}]';
   var actCnt  = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[cnt:"+ expCnt +", recs:"+ expRecs +"]", 
                        "[cnt:"+ actCnt +", recs:"+ actRecs +"]" );
   }
   println( "cl records: "+ actRecs );
   
}