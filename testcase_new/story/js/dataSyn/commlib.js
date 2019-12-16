// create WORKDIR in local host
var saveRecordsDir = WORKDIR + "/datasyn/"
commMakeDir( COORDHOSTNAME, saveRecordsDir );
var cmd = new Cmd();

/************************************
@Description: Insert data to SequoiaDB
@parameter:
   dbcl: db.cs.cl
   insertNum: insert nums
@return: the insert datas
@author��wuyan 2018/12/27
**************************************/
function insertData ( dbcl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 10000; }

   try
   {
      println( "---begin to insert data" );
      var docs = [];
      for( var i = 0; i < insertNum; ++i )
      {
         var no = i;
         var str = getRandomString( 100 ) + "teststr_" + i;
         var inta = i;
         var fc = i + 0.5896;
         var objs = { "no": no, "str": str, "inta": inta, "fc": fc };

         // insert date        
         docs.push( objs );
      }
      dbcl.insert( docs );
      return docs;
      println( "--end insert data" )
   }
   catch( e )
   {
      throw buildException( "insertDate()", e );
   }
}

/************************************
@Description: buckInsert data to SequoiaDB
@parameter:
   dbcl: db.cs.cl
   insertNum: the number of inserts must be an integer of 1w
@return: the insert datas
@author��wuyan 2018/12/27
**************************************/
function buckInsertData ( dbcl, insertNums, beginNums )
{
   if( undefined == beginNums ) { beginNums = 0; }
   try
   {
      println( "---begin to buckInsert data." );
      var batchNums = 10000;
      var recs = [];
      var times = insertNums / batchNums;

      for( var k = 0; k < times; k++ )
      {
         var doc = [];
         for( var i = 0; i < batchNums; ++i )
         {
            var count = beginNums++
            var no = count;
            var str = getRandomString( 100 ) + "teststr_" + count;
            var inta = count;
            var fc = count + 0.7898;
            var objs = { "no": no, "str": str, "inta": inta, "fc": fc };
            doc.push( objs );
            recs.push( objs );
         }
         dbcl.insert( doc );
      }
      println( "---end bulkInsert data." )
      return recs;
   }
   catch( e )
   {
      throw buildException( "bulkInsertDate()", e );
   }
}


function getRandomFloat ( min, max )
{
   var range = max - min;
   var value = min + Math.random() * range;
   return value;
}

function getRandomString ( len ) 
{
   var strLen = parseInt( Math.random() * len );
   var str = "";
   var chars = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
   var maxPos = chars.length;
   for( var i = 0; i < strLen; i++ )
   {
      str += chars.charAt( Math.floor( Math.random() * maxPos ) );
   }
   return str;
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkRec ( rc, expRecs, filterId, fileNameId )
{
   println( "---begin to check data context." );
   //get actual records to array
   var actRecs = [];

   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      saveCheckRecords( fileNameId, actRecs, expRecs );
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
         if( !compareObj( actRec[f], expRec[f], filterId ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs[i] ) + "\n\nexpect recs= " + JSON.stringify( expRecs[i] ) );
            saveCheckRecords( fileNameId, actRecs, expRecs );
            throw buildException( "checkRec()", "check actRecs fail!" );
         }
      }
   }

}

function compareObj ( lobj, robj, ignoreId )
{
   if( typeof ( lobj ) === "object" &&
      typeof ( robj ) === "object" )
   {
      if( lobj === null && robj === null ) return true;
      if( lobj.constructor !== robj.constructor ) return false;
      var _idNum = 0;
      for( key in lobj )
      {
         if( ignoreId && key === "_id" ) { _idNum = 1; continue };
         if( undefined === robj[key] ) return false;
         if( !compareObj( lobj[key], robj[key] ) ) return false;
      }

      var lkeys = Object.getOwnPropertyNames( lobj );
      var rkeys = Object.getOwnPropertyNames( robj );
      if( lkeys.length !== rkeys.length + _idNum ) return false;
      return true;
   }
   else if( lobj === robj )
   {
      return true;
   } else
   {
      return false;
   }
}

/******************************************************************************
@Description : check data from cl
@parameter:
   sdb: 
   csName: 
   clName: 
   sortCOnd:sort condition,eg:{a:1}
   expRecs:export datas
   testcaseId:castID,used to fileName of save datas.eg:16992
   filterID:whether to filter id fields
   findCond:query condition,eg:{a:{$gt:0}}
******************************************************************************/
function checkDataContent ( sdb, csName, clName, sortCond, expRecs, testcaseId, filterId, findCond )
{
   if( undefined == filterId ) { filterId = true; }
   if( undefined == findCond ) { findCond = null; }
   println( "---begin to check result from master node." );
   sdb.setSessionAttr( { PreferedInstance: "M" } );
   var dbcl = sdb.getCS( csName ).getCL( clName );
   var rc = dbcl.find( findCond ).sort( sortCond );
   checkRec( rc, expRecs, filterId, testcaseId );
}

/******************************************************************************
@Description : check inpect result
@input:         csName
                clName
                checkTimes
******************************************************************************/
function checkInspectResult ( csName, clName, checkTimes )
{
   println( "---begin to check data consistency." );
   if( typeof ( checkTimes ) == "undefined" ) { checkTimes = 20; }

   var inspectBinFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin";
   var inspectReportFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin.report";
   var installPath = commGetInstallPath();
   var inspectCommand = installPath + "/bin/sdbinspect" + " -d " + COORDHOSTNAME + ":" + COORDSVCNAME + " -c " + csName + " -l " + clName + " -o " + inspectBinFile + " -t " + checkTimes;
   try 
   {
      // exec sdbinspect 
      cmd.run( inspectCommand );
      var info = cmd.run( "tail -n 1 " + inspectReportFile );
      var actResult = info.split( "\n" )[0].split( "\:" )[1].trim();
      var expectRusult = "exit with no records different";
      // compare result
      if( actResult == expectRusult )
      {
         // remove report files
         cmd.run( "rm -f " + inspectBinFile );
         cmd.run( "rm -f " + inspectReportFile );
         println( "---check consistency success!" );
      }
      else
      {
         println( "---check consistency fail, cl name: " + csName + "." + clName );
      }
   }
   catch( e ) 
   {
      throw buildException( "checkConsistency", "check consistency fail", "fail",
         e, e );
   }
}

/************************************
@Description: save records to file
@parameter:
   fileNameId: caseId,save the file name is caseId
   actRecs: actual query datas
   expRecs: expect datas
@author��wuyan 2018/12/27
**************************************/
function saveCheckRecords ( fileNameId, actRecs, expRecs )
{
   var fileName1 = saveRecordsDir + "actRecs_" + fileNameId + ".txt";
   var file1 = new File( fileName1 );
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      file1.write( JSON.stringify( actRecs[i] ) + "\n" );
   }
   file1.close();

   var fileName2 = saveRecordsDir + "expRecs_" + fileNameId + ".txt";
   var file2 = new File( fileName2 );
   for( var i in expRecs )
   {
      var expRec = expRecs[i];
      file2.write( JSON.stringify( expRecs[i] ) + "\n" );
   }
   file2.close();

}

/*****************************************************************
*@Description: get all nodes of all groups
@input:        groups
******************************************************************/
function getNodesInGroups ( groups )
{
   var datas = new Array();

   for( var i = 0; i < groups.length; ++i )
   {
      datas[i] = Array();

      var rg = db.getRG( groups[i] );
      var rgDetail = eval( "(" + rg.getDetail().toArray()[0] + ")" );
      var nodesInGroup = rgDetail.Group;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var hostName = nodesInGroup[j].HostName;
         var serviceName = nodesInGroup[j].Service[0].Name;
         datas[i][j] = new Sdb( hostName, serviceName );
      }

   }
   return datas;
}

/*****************************************************************
*@Description: check lsn consistency
@input:         groups
                primaryNodeLSNs
******************************************************************/
function checkLSN ( groups, primaryNodeLSNs )
{
   var slaveNodeLSNs = getSlaveNodeLSNs( groups );

   var checkLSN = true;
   for( var i = 0; i < slaveNodeLSNs.length; ++i )
   {
      for( var j = 0; j < slaveNodeLSNs[i].length; ++j )
      {
         if( primaryNodeLSNs[i][0] > slaveNodeLSNs[i][j] )
         {
            checkLSN = false;
            return checkLSN;
         }
      }
   }

   return checkLSN;
}

/*****************************************************************
*@Description: get lsn of primary node
@input:        groups
******************************************************************/
function getPrimaryNodeLSNs ( groups )
{

   var datas = getNodesInGroups( groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( isPrimary )
         {
            LSNs[i][0] = completeLSN;
            break;
         }
      }
   }

   return LSNs;
}

/*****************************************************************
*@Description: get lsn of slave node
@input:        groups
******************************************************************/
function getSlaveNodeLSNs ( groups )
{
   var datas = getNodesInGroups( groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      var f = 0;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( !isPrimary )
         {
            LSNs[i][f++] = completeLSN;
         }
      }
   }

   return LSNs;
}

/*****************************************************************
*@Description : check consistency: LSNs consistency of all nodes
@input:         csName
                clName
******************************************************************/
function checkConsistency ( csName, clName )
{
   // check LSN consistency
   if( csName == null ) { var csName = "UNDEFINED"; }
   if( clName == null ) { var clName = "UNDEFINED"; }

   var groups = commGetCLGroups( db, csName + "." + clName );

   //the longest waiting time is 600S
   var lsnFlag = false;
   var timeout = 600;
   var doTimes = 0;

   //get primary nodes
   var primaryNodeLSNs = getPrimaryNodeLSNs( groups );
   while( true )
   {
      lsnFlag = checkLSN( groups, primaryNodeLSNs );
      if( !lsnFlag )
      {
         if( doTimes < timeout )
         {
            ++doTimes;
            sleep( 1000 );
         }
         else
         {
            throw "check lsn time out";
         }
      }
      else 
      {
         break;
      }
   }

   println( "---check consistency success!" );
}

function DataSyncTestCase ( db, csName, clName )
{
   this.db = db;
   this.csName = csName;
   this.clName = clName;
}

DataSyncTestCase.prototype.setUp = function()
{
   commDropCL( this.db, this.csName, this.clName, true, true );
   var options = { Compressed: true };
   this.dbcl = commCreateCL( this.db, this.csName, this.clName, options );
}

DataSyncTestCase.prototype.tearDown = function()
{
   commDropCL( this.db, this.csName, this.clName, true, true );
}

DataSyncTestCase.prototype.checkResult =
   function( sortCond, expRecs, testcaseId, filterId, findCond )
   {
      if( undefined == filterId ) { filterId = true; }
      if( undefined == findCond ) { findCond = null; }
      checkDataContent( this.db, this.csName, this.clName, sortCond, expRecs, testcaseId, filterId, findCond );
      checkConsistency( this.csName, this.clName );
      checkInspectResult( this.csName, this.clName );
   }


function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   testcase = new DataSyncTestCase( db, csName, clName );
   testcase.setUp();
   testcase.execTest();
   testcase.tearDown();
}
