/************************************************************************
*@Description:  seqDB-8530:导入csv文件时指定decimal类型且指定精度数据校验
*@Author:           2016-8-3  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_8530" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"8530.csv";
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
   file.write( "1,9223372036854775808\n"
              +"2,-9223372036854775809\n"
              +"3, 92233720368547758079223372036854775807\n"
              +"4,-92233720368547758079223372036854775807\n"
              +"5, 92233720368547758089223372036854775808\n"
              +"6,-92233720368547758089223372036854775808" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                  +' -c '+ csName +' -l '+ clName 
                  +' --type csv --fields "a,b"'
                  +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcObj = rc.split("\n");
   
   //check import results
   var expParseRecords    = "parsed records: 6";
   var expParseFailure    = "parse failure: 0";
   var expImportedRecords = "imported records: 6";
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
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find({b:{$type:1,$et:100}},{_id:{$include:0}}).sort({a:1});
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 6;
   var expRecs = '[{"a":1,"b":{"$decimal":"9223372036854775808"}},{"a":2,"b":{"$decimal":"-9223372036854775809"}},{"a":3,"b":{"$decimal":"92233720368547758079223372036854775807"}},{"a":4,"b":{"$decimal":"-92233720368547758079223372036854775807"}},{"a":5,"b":{"$decimal":"92233720368547758089223372036854775808"}},{"a":6,"b":{"$decimal":"-92233720368547758089223372036854775808"}}]';
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