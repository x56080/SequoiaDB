/************************************************************************
*@Description:    seqDB-5500:自定义时间戳格式导入时间戳数据（--timestampfmt），
                        其中包含通配符（*）和特殊字符（任意UTF-8字符）
*@Author:   2016-7-29  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5500" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5500.csv";
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
   file.write( '1,YYYY-MM-DD-HH.mm.ss.ffffff,1902-01-01-00.00.00.000000' +"\n"
			    + '2,YYYY/MM/DD HH:mm:ss:ffffff,2037/12/31 23:59:59:999999' +"\n"
			    + '3,HH.mm.ss.ffffff YYYY-MM-DD,00.00.00.000000 1996-02-29' );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var tmpRec = csName +"_"+ clName +"*.rec";
   var timestampfmt = ["--timestampfmt 'YYYY-MM-DD-HH.mm.ss.ffffff'", 
                       "--timestampfmt 'YYYY/MM/DD HH:mm:ss:ffffff'", 
                       "--timestampfmt 'HH.mm.ss.ffffff YYYY-MM-DD'"]
   for( i=0; i<timestampfmt.length; i++ )
   {
      //remove rec file
      cmd.run( "rm -rf "+ tmpRec );
      
      //import operation
      var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                        +' -c '+ csName +' -l '+ clName 
                        +' --type csv --fields "num int,desc string,v1 timestamp" '
                        +timestampfmt[i]
                        +' --file '+ imprtFile;
      println( imprtOption );
      var rc = cmd.run( imprtOption );
      println( rc );
      
      //check import results
      var rcObj = rc.split("\n");
      var expParseRecords    = "parsed records: 1";
      var expParseFailure    = "parse failure: 2";
      var expImportedRecords = "imported records: 1";
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
      var expFailedNum = 2;
      if( expFailedNum !== actFailedNum )
      {
         throw buildException( "checkCLdata", null, "[find]", 
                           "[failedRecs:"+ expFailedNum +"]", 
                           "[failedRecs:"+ actFailedNum +"]" );
      }
   }
   
   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({v1:{$type:1,$et:17}},{_id:{$include:0}}).sort({num:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 3;  
   var expRecs = '[{"num":1,"desc":"YYYY-MM-DD-HH.mm.ss.ffffff","v1":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"num":2,"desc":"YYYY/MM/DD HH:mm:ss:ffffff","v1":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"num":3,"desc":"HH.mm.ss.ffffff YYYY-MM-DD","v1":{"$timestamp":"1996-02-29-00.00.00.000000"}}]';
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