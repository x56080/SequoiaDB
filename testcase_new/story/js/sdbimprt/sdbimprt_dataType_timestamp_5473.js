/************************************************************************
*@Description:    seqDB-5473:导入数据时指定的数据类型为timestamp，实际数据为支持转换的数据类型
                    int 	long 	string 	date 	timestamp
                  seqDB-5474/seqDB-5475
*@Author:   2016-7-29  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5473" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = testCaseDir +"dataFile/allDataType.csv";
      importData( csName, clName, imprtFile );
   	
      checkCLData( cl );
      cleanCL( csName, clName );
   }
      catch(e)
   {
   	throw e;
   }
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   //remove rec file
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf "+ tmpRec );
   
   ////cat import file
   //var fileInfo = cmd.run( "cat "+ imprtFile );
   //println( imprtFile +"\n" + fileInfo +"\n" );
   
   //import operation
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --fields "num int,type string,v1 timestamp,v2 timestamp"'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   //check import results
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 2";
   var expParseFailure    = "parse failure: 34";
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
   var actFailedNum = cmd.run( "cat -v "+ rec ).split("\n").length - 1;
   println( rec +"\nrecords number: "+ actFailedNum );
   var expFailedNum = 34;
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
   
   var rc = cl.find({$and:[{v1:{$type:1,$et:17}},{v2:{$type:1,$et:17}}]},{_id:{$include:0}}).sort({num:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 2;  
   var expRecs = '[{"num":11,"type":"timestamp","v1":{"$timestamp":"1902-01-01-00.00.00.000000"},"v2":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"num":40,"type":"timestampStr","v1":{"$timestamp":"1902-01-01-00.00.00.000000"},"v2":{"$timestamp":"2037-12-31-23.59.59.999999"}}]';
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