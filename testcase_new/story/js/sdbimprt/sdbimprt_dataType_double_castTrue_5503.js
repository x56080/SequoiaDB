/************************************************************************
*@Description:    seqDB-5503:自定义允许精度丢失/数值溢出（如cast取值为true）
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_5503" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"5503.csv";
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
   file.write( "1,-2147483649\n"
              +"2,2147483648\n"
              +"3,-9223372036854775809\n"
              +"4, 9223372036854775808\n"
              +"5,-1.8E+308\n"
              +"6, 1.8E+308\n" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                  +' -c '+ csName +' -l '+ clName 
                  +' --type csv --fields "a int,v1 double" --cast true'
                  +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcObj = rc.split("\n");
   
   var expParseRecords    = "parsed records: 6";
   var expParseFailure    = "parse failure: 0";
   var expImportedRecords = "imported records: 6";
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
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({v1:{$type:1,$et:1}},{_id:{$include:0}}).sort({a:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 6;
   var expRecs = '[{"a":1,"v1":-2147483649},{"a":2,"v1":2147483648},{"a":3,"v1":-9223372036854776000},{"a":4,"v1":9223372036854776000},{"a":5,"v1":-Infinity},{"a":6,"v1":Infinity}]';
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