/************************************************************************
*@Description:  seqDB-5470:导入数据时指定的数据类型为double，实际数据取值为带符号的double数值
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5470" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5470.csv";
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
   file.write( "1,+0058.00\n2,+0058.\n3,-00000020000.00" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                  +' -c '+ csName +' -l '+ clName 
                  +' --type csv --fields "a int,b double"'
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
   var expRecs = '[{"a":1,"b":58},{"a":2,"b":58},{"a":3,"b":-20000}]';
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