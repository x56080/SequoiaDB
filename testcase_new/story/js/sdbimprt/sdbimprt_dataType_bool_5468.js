/************************************************************************
*@Description:  seqDB-5468:导入数据时指定的数据类型为bool(boolean)，实际字段值为空
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5468" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5468.csv";
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
   file.write( '1,"string","1", "0"' +"\n"
             + '2,"string",""' +"\n"
             + '3,null,,' );
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
                  +' --type csv --fields "num int,type string,v1 bool,v2 bool"'
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
   
   //check failed records
   var rec = cmd.run( "ls "+ tmpRec ).split("\n")[0];
   var actFailedNum = cmd.run( "cat "+ rec ).split("\n").length - 1;
   println( rec +"\nrecords number: "+ actFailedNum );
   var expFailedNum = 1;
   if( expFailedNum !== actFailedNum )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[failedRecs:"+ expFailedNum +"]", 
                        "[failedRecs:"+ actFailedNum +"]" );
   }
   
   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({},{_id:{$include:0}}).sort({num:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 2;
   var expRecs = '[{"num":1,"type":"string","v1":true,"v2":false},{"num":3,"type":"null","v1":null}]';
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