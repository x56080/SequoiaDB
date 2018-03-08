/************************************************************************
*@Description:   seqDB-5437:导入不在sharding分区范围内的记录，并按分区信息重新打包记录
                 seqDB-5543
*@Author:        2016-7-14  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      if( commIsStandalone( db ) )
      {
      	println(" Deploy mode is standalone!");
			return;
      }
      
      var csName     = COMMCSNAME;
      var mainclName = COMMCLNAME+"_5437_mainCL";
      var subclName  = COMMCLNAME+"_5437_subCL";
      //create mainCL
      var opt1   = {ShardingKey:{a:1},IsMainCL:true} ;
      var mainCL = readyCL( csName, mainclName, opt1, "[mainCL]" );
      //create subCL
      var opt2   = {ShardingKey:{a:1},ShardingType:"hash",ReplSize:0} ;
      var subCL = readyCL( csName, subclName, opt2, "[subCL]" );
      //attach cl
      var options = {LowBound:{"a":1},UpBound:{ "a":10}} ; 
      mainCL.attachCL( csName+"."+subclName, options );
      
      var imprtFile = tmpFileDir +"5437.csv";
      readyData( imprtFile );
      importData( csName, mainclName, imprtFile );
   	
      checkCLData( mainCL );
      cleanCL( csName, subclName );
      cleanCL( csName, mainclName );
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
   file.write( "a int\n1\n6\n9\n0\n10" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importData( csName, clName, imprtFile )
{
   println("\n---Begin to import data and check exec result.");
   //remove rec file
   var tmpRec = csName +"_"+ clName +"*.rec";
   cmd.run( "rm -rf "+ tmpRec );
   
   var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                     +' -c '+ csName +' -l '+ clName 
                     +' --type csv --headerline true --sharding true -n 2'
                     +' --file '+ imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   
   var rcObj = rc.split("\n");
   var expParseRecords    = "parsed records: 5";
   var expImportedRecords = "imported records: 2";
   var expImportFailure   = "import failure: 3";
   var actParseRecords    = rcObj[0];
   var actImportedRecords = rcObj[4];
   var actImportFailure   = rcObj[5];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords 
     || expImportFailure !== actImportFailure )
   {
      throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expImportedRecords +", "+ expImportFailure +"]", 
                        "["+ actParseRecords +", "+ actImportedRecords +", "+ actImportFailure +"]" );
   }
   
   var rec = cmd.run( "ls "+ tmpRec ).split("\n")[0];
   var tmpRecs = cmd.run( "cut -c 49-56 "+ rec ).split("\n");
   var failedRecs = String( [ tmpRecs[0], tmpRecs[1], tmpRecs[2] ] );
   println( rec +"\n"+ failedRecs );
   var expRecRecs = ' "a": 9 , "a": 0 , "a": 10';
   var actRecRecs = failedRecs;
   if( expRecRecs !== actRecRecs )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[failedRecs:"+ expRecRecs +"]", 
                        "[failedRecs:"+ actRecRecs +"]" );
   }
   
   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
   
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
   
   var expCnt  = 2;
   var expRecs = '[{"a":1},{"a":6}]';
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
