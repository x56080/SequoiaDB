/************************************************************************
*@Description:   seqDB-6387:指定autotimstamp参数导入timestamp类型数据
                    int 	long 	string 	date 	timestamp
*@Author:   2016-7-29  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_6387" ;
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
                     +' --type csv --fields "num int,type string,v1 autotimestamp,v2 autotimestamp"'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   //check import results
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 10";
   var expParseFailure    = "parse failure: 26";
   var expImportedRecords = "imported records: 10";
   var actParseRecords    = rcObj[0];
   var actParseFailure    = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseFailure !== actParseFailure 
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
   var expFailedNum = 26;
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
   
   //get local time
   var expT1 = cmd.run('date -d@"-2147414400" +"%Y-%m-%d-%H.%M.%S.000000"').split("\n")[0];
   
   var expCnt  = 10;  
   var expRecs = '[{"num":1,"type":"int","v1":{"$timestamp":"1969-12-07-11.28.36.352000"},"v2":{"$timestamp":"1970-01-26-04.31.23.647000"}},{"num":5,"type":"number","v1":{"$timestamp":"1969-12-07-11.28.36.351000"},"v2":{"$timestamp":"1970-01-26-04.31.23.648000"}},{"num":11,"type":"timestamp","v1":{"$timestamp":"1902-01-01-00.00.00.000000"},"v2":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"num":21,"type":"dateMS","v1":{"$timestamp":"1958-01-12-17.54.14.057000"},"v2":{"$timestamp":"1978-01-12-05.31.11.999000"}},{"num":22,"type":"timestampMS","v1":{"$timestamp":"'+ expT1 +'"},"v2":{"$timestamp":"2038-01-18-23.59.59.000000"}},{"num":31,"type":"intStr","v1":{"$timestamp":"1969-12-07-11.28.36.352000"},"v2":{"$timestamp":"1970-01-26-04.31.23.647000"}},{"num":35,"type":"numberStr","v1":{"$timestamp":"1969-12-07-11.28.36.351000"},"v2":{"$timestamp":"1970-01-26-04.31.23.648000"}},{"num":40,"type":"timestampStr","v1":{"$timestamp":"1902-01-01-00.00.00.000000"},"v2":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"num":50,"type":"int","v1":{"$timestamp":"1969-12-07-11.28.36.351000"},"v2":{"$timestamp":"1970-01-26-04.31.23.648000"}},{"num":57,"type":"dateMSStr","v1":{"$timestamp":"1958-01-12-17.54.14.056000"},"v2":{"$timestamp":"1978-01-12-05.31.12.000000"}}]';
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