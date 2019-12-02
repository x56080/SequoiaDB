/*******************************************************************************
@Description : common functions
@Modify list : 2016-3-28  Ting YU  Init
*******************************************************************************/
function Collection( csName, clName, opt )
{
   this.csName = csName; 
   this.clName = clName; 
   
   this.create = 
   function()
   {
      println( "---begin to create cl = " + csName + '.' + clName ); 
      commDropCL( db, csName, clName, true, true, "drop cl in begin" ); 
      this.cl = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" ); 
      return this.cl; 
   }
   
   this.getSelf = 
   function()
   {
      return this.cl; 
   }
   
   this.istRandomRecs = 
   function( recNum, dataTypes, fieldNames )
   {
      println( "---begin to insert" ); 
      var rd = new commDataGenerator(); 
      var recs = rd.getRecords( recNum, dataTypes, fieldNames ); 
      db.getCS( csName ).getCL( clName ).insert( recs ); 
   }
   
   this.istRecs = 
   function( recs )
   {
      println( "---begin to insert" ); 
      db.getCS( csName ).getCL( clName ).insert( recs ); 
   }
   
   this.getGroups = 
   function()
   {
      println( "---begin to get groups of cl" ); 
      var clFullName = csName + '.' + clName; 
      return commGetCLGroups( db, clFullName ); 
   }
   
   this.createIndex = 
   function( indexName, indexDef, isUnique, enforced )
   {
      if( isUnique === undefined ){ isUnique = false; }
      if( enforced === undefined ){ enforced = false; }
      println( "---begin to create index" ); 
      db.getCS( csName ).getCL( clName ).createIndex( indexName, indexDef, isUnique, enforced ); 
   }
   
}

function select2RG()
{
   var dataRGInfo = commGetGroups( db ); 
   var rgsName = {}; 
   rgsName.srcRG = dataRGInfo[0][0]["GroupName"]; //source group
   rgsName.tgtRG = dataRGInfo[1][0]["GroupName"]; //target group
   
   return rgsName; 
}

function checkRec( rc, expRecs )
{
   //get actual records to array
   var actRecs = []; 
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() ); 
   }
   
   //check count
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs )+ "\n\nexpect recs= " + JSON.stringify( expRecs ) ); 
      throw buildException( "check count", null, "", 
      expRecs.length, actRecs.length ); 
   }
   
   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i]; 
      var expRec = expRecs[i]; 
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] )!== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " +( parseInt( i )+ 1 )+ "th record, in field '" + f + "'" ); 
            println( "\nactual recs in cl= " + JSON.stringify( actRecs )+ "\n\nexpect recs= " + JSON.stringify( expRecs ) ); 
            throw buildException( "checkRec()", "rec ERROR" ); 
         }
      }
   }
}

function checkExplain( rc, expIdxName )
{
   var plan = rc.explain().current().toObj(); 
   var expScanType = "ixscan"; 
   if( expIdxName == "" ){ expScanType = "tbscan"; }
   
   if( plan.ScanType !== expScanType )
   {
      throw buildException( "checkExplain()", null, "query.explain().ScanType", 
      expScanType, plan.ScanType ); 
   }
   if( plan.IndexName !== expIdxName )
   {
      throw buildException( "checkExplain()", null, "query.explain().IndexName", 
      expIdxName, plan.IndexName ); 
   }
}

function select2RG()
{
   var dataRGInfo = commGetGroups( db ); 
   var rgsName = {}; 
   rgsName.srcRG = dataRGInfo[0][0]["GroupName"]; //source group
   rgsName.tgtRG = dataRGInfo[1][0]["GroupName"]; //target group
   
   return rgsName; 
}
/****************************************************************************
@discription: generate curl command
@parameter:
curlPara: eg:[ "cmd=query", "name=foo.bar" ]
@return object ex:
object.errno: 0
object.records: [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }]
object.curlCommand: 'curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar"'
*****************************************************************************/
function runCurl( curlPara, expErrno )
{
   var curlCommand = geneCurlCommad( curlPara ); //generate curl command
   
   //run
   try
   {
      var cmd = new Cmd(); 
      var rtnInfo = cmd.run( curlCommand ); 
   }
   catch( e )
   {
      throw buildException( "runCurl():run", null, 
      curlCommand, "excute succed", "excute fail" ); 
   }
   
   //check return value
   var curlInfo = resolveRtnInfo( rtnInfo ); 
   curlInfo["curlCommand"] = curlCommand; 
   
   if( curlInfo.errno !== expErrno )
   {
      println( "\nreturn information: " + rtnInfo ); 
      throw buildException( "runCurl():check return value", null, 
      curlInfo.curlCommand, expErrno, curlInfo.errno ); 
   }
   
   return curlInfo; 
}

/****************************************************************************
@discription: generate curl command
@parameter:
curlPara: eg:[ "cmd=query", "name=foo.bar" ]
@return string
eg:'curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar" 2 > /dev/null'
*****************************************************************************/
function geneCurlCommad( curlPara )
{
   //head of curl command
   var restPort = parseInt( COORDSVCNAME, 10 )+ 4; 
   var curlHead = 'curl http://' + COORDHOSTNAME + ':' + restPort + '/ -d'; 
   
   //main of curl command
   var curlMain = "'"; //begin with ', not with "
   for( var i in curlPara )
   {
      curlMain += curlPara[i]; 
      curlMain += '&'; 
   }
   curlMain = curlMain.substring( 0, curlMain.length-1 ); //remove last character &
   curlMain += "'"; //end with ', not with "
   
   //tail of curl command
   var curlTail = '2>/dev/null'; 
   
   var curlCommand = curlHead + ' ' + curlMain + ' ' + curlTail; 
   return curlCommand; 
}

/****************************************************************************
@discription: resolve returned infomation by curl commad
@parameter:
rtnInfo: eg:{ "errno": 0 }{ "_id": 1, "a": 1 }{ "_id": 2, "a": 2 }
@return object ex:
object.errno: 0
object.records: [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }]
*****************************************************************************/
function resolveRtnInfo( rtnInfo )
{
   var rtn = {}; 
   
   //change to array
   var rtnInfoArr = rtnInfo.replace( /}{/g, "}\n{" ).split( "\n" ); 
   
   //get errno
   var errnoStr = rtnInfoArr[0].slice( 11, 15 ); 
   var errno = parseInt( errnoStr, 10 ); 
   rtn["errno"] = errno; 
   
   //get other
   rtnInfoArr.shift(); //abandon rtnInfoArr[0]
   rtn["rtnJsn"] = rtnInfoArr; 
   
   
   return rtn; 
}
