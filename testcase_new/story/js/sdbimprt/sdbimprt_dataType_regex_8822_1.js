/************************************************************************
*@Description:    seqDB-8822:导入正则表达式数据包含各种转义字符(csv)
*@Author:            2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_8822" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"8822.csv";
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
   file.write( '1,"/^(?:\\\\x22?\\\\x5C[\\\\x00-\\\\x7E]\\\\x22?)|(?:\\\\x22?[^\\\\x5C\\\\x22]\\\\x22?)|(?:\\\\x22?[^\\\\x5C\\\\x22]\\\\x22?)/"' + "\n" 
   + '2,"^\\d+$"' + "\n" 
   + '3,"\\f\\b\\r\\n"' );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --fields "a int,b regex"'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 3";
   var expImportedRecords = "imported records: 3";
   var actParseRecords    = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords 
    || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expImportedRecords +"]", 
                        "["+ actParseRecords +", "+ actImportedRecords +"]" );
   }
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
   
   var expCnt  = 3;
   var expRecs = '[{"a":1,"b":{"$regex":"^(?:\\\\\\\\x22?\\\\\\\\x5C[\\\\\\\\x00-\\\\\\\\x7E]\\\\\\\\x22?)|(?:\\\\\\\\x22?[^\\\\\\\\x5C\\\\\\\\x22]\\\\\\\\x22?)|(?:\\\\\\\\x22?[^\\\\\\\\x5C\\\\\\\\x22]\\\\\\\\x22?)","$options":""}},{"a":2,"b":{"$regex":"^\\\\d+$","$options":""}},{"a":3,"b":{"$regex":"\\\\f\\\\b\\\\r\\\\n","$options":""}}]';
   var actCnt  = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[cnt:"+ expCnt +", recs:"+ expRecs +"]", 
                        "[cnt:"+ actCnt +", recs:"+ actRecs +"]" );
   }
   //println( "cl records: "+ actRecs );
   
}