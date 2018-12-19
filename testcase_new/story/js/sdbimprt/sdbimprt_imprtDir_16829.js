/*******************************************************************************
*@Description:   seqDB-16829:headerline=true 时导入目录下多个文件
*@Author:        2018-12-19  wangkexin
********************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_16829" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile1 = tmpFileDir +"16829_1.csv";
	  var imprtFile2 = tmpFileDir +"16829_2.csv";
      readyData( imprtFile1, imprtFile2 );
      importData( csName, clName );
   	
      checkCLData( cl );
      cleanCL( csName, clName );
   }
      catch(e)
   {
   	throw e;
   }
}

function readyData( imprtFile1, imprtFile2 )
{
   println("\n---Begin to ready data.");
   
   var file = fileInit( imprtFile1 );
   file.write('name, age, country\n"Jack",18,"China"\n"Mike",20,"USA"');
   var fileInfo = cmd.run( "cat "+ imprtFile1 );
   println( imprtFile1 +"\n" + fileInfo );
   file.close();
   
   var file = fileInit( imprtFile2 );
   file.write('name, age, country\n"Jack1",181,"China1"\n"Mike1",201,"USA1"');
   var fileInfo = cmd.run( "cat "+ imprtFile2 );
   println( imprtFile2 +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   //remove rec file
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf "+ tmpRec );
   
   //import operation
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv '
                     +' --file '+ tmpFileDir
					 +' --headerline=true ';
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   //check import results
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 4";
   var expImportedRecords = "imported records: 4";
   var actParseRecords    = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords 
    || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expImportedRecords +"]", 
                        "["+ actParseRecords +", "+ actImportedRecords +"]" );
   }
    
   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({},{_id:{$include:0}});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 4;  
   var expRecs = '[{"name":"Jack1","age":181,"country":"China1"},{"name":"Mike1","age":201,"country":"USA1"},{"name":"Jack","age":18,"country":"China"},{"name":"Mike","age":20,"country":"USA"}]';
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
