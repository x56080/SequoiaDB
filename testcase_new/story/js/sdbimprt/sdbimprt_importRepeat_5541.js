/************************************************************************
*@Description:    seqDB-5541:重复导入
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName  = COMMCSNAME;
      var clName  = COMMCLNAME+"_5541" ;
      var idxName = "idx_a" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5541.csv";
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
   file.write( "1,test\n"
              +"2,test" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var times = 2;
   for( i=0;i<times; i++ )
   {
      var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --fields "a int,b string"'
                     +' --file '+ imprtFile;
      println( imprtOption );
      var rc = cmd.run( imprtOption );
      println( rc );
      var rcObj = rc.split("\n");
      
      var expParseRecords    = "parsed records: 2";
      var expImportedRecords = "imported records: 2";
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
   
   var expCnt  = 4;
   var expRecs = '[{"a":1,"b":"test"},{"a":1,"b":"test"},{"a":2,"b":"test"},{"a":2,"b":"test"}]';
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