/************************************************************************
*@Description:   seqDB-5458:导入数据时指定数据类型为int，实际数值为带空格的字符串
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5458" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5458.csv";
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
   file.write( '1,"string","  123  ", " -123 ", " -0 ", " 0 "' );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                  +' -c '+ csName +' -l '+ clName 
                  +' --type csv --fields "num int,type string,v1 int,v2 int,v3 int,v4 int"'
                  +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcObj = rc.split("\n");
   
   var expParseRecords    = "parsed records: 1";
   var expImportedRecords = "imported records: 1";
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
   
   var rc = cl.find({$and:[{v1:{$type:1,$et:16}},{v2:{$type:1,$et:16}}]},{_id:{$include:0}}).sort({num:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 1;
   var expRecs = '[{"num":1,"type":"string","v1":123,"v2":-123,"v3":0,"v4":0}]';
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