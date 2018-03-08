/************************************************************************
*@Description:  seqDB-6762:导入非utf-8格式的文件数据
*@Author:            2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_6762" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = testCaseDir +"dataFile/NOT_UTF-8";
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
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --fields "a" --sparse true'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   var rcObj = rc.split("\n");
   var expParseRecords    = "parse failure: 1";
   var expImportedRecords = "imported records: 0";
   var actParseRecords    = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords 
    || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expImportedRecords +"]", 
                        "["+ actParseRecords +", "+ actImportedRecords +"]" );
   }
   
   // clean tmpRec
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data after the sdbimprt operation.");
   
   var actCnt = 0; 
   var expCnt = Number( cl.count() );
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[cl count]", 
                        "[cnt:"+ expCnt +"]", 
                        "[cnt:"+ actCnt +"]" );
   }
}