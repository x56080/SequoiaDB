/************************************************************************
*@Description:   seqDB-8531:导入csv文件指定decimal类型且指定精度命令行验证
*@Author:           2016-8-3  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_8531" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"8531.csv";
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
   file.write( "1" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var decimalFmt = [ "a decimal(1001,1)",
                      "a decimal(10,11)",
                      "a decimal(0,0)",
                      "a decimal(10,10)" ];
   for( i = 0; i < decimalFmt.length; i++ )
   {
      var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --fields "'+ decimalFmt[i]
                     +'" --file '+ imprtFile;
      println( imprtOption );
      var rc = cmd.run( imprtOption );
      println( rc );
      var rcObj = rc.split("\n");
      
      //check import results
      if( i < decimalFmt.length-1 )
      {
         var expParseRecords    = "parsed records: 0";
         var expParseFailure    = "parse failure: 0";
         var expImportedRecords = "imported records: 0";
         var actParseRecords    = rcObj[1];
         var actParseFailure    = rcObj[2];
         var actImportedRecords = rcObj[5];
      }
      else
      {
         var expParseRecords    = "parsed records: 0";
         var expParseFailure    = "parse failure: 1";
         var expImportedRecords = "imported records: 0";
         var actParseRecords    = rcObj[0];
         var actParseFailure    = rcObj[1];
         var actImportedRecords = rcObj[4];
      }
      
      if( expParseRecords !== actParseRecords || expParseFailure !== actParseFailure 
       || expImportedRecords !== actImportedRecords )
      {
         throw buildException( "importData", null, "[sdbimprt results]", 
                           "["+ expParseRecords +", "+ expParseFailure +", "+ expImportedRecords +"]", 
                           "["+ actParseRecords +", "+ actParseFailure +", "+ actImportedRecords +"]" );
      }
   }
   
   // clean tmpRec
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find();
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 0;
   var actCnt  = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[cnt:"+ expCnt +"]", 
                        "[cnt:"+ actCnt +"]" );
   }
   
}