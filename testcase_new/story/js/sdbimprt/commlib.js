/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2016/7/14  XiaoNi Huang Init
*******************************************************************************/
println();
var testCaseDir = initTestCaseDir();
var tmpFileDir = WORKDIR + "/sdbimprt/";
println( "tmpFileDir  = " + tmpFileDir );
var localPath = null;

var cmd = cmdInit();
var installDir = getInstallDir();   //eg:/opt/sequoiadb
readyTmpDir();


/* ****************************************************
@description: get testcase director
@return: testcase director
**************************************************** */
function initTestCaseDir ()
{
   if( typeof ( TESTCASEDIR ) == "undefined" ) 
   {
      var testCaseDir = './testcase_new/story/js/sdbimprt/';
   }
   else
   {
      //TESTCASEDIR default: ....../testcases/hlt/js_testcases/js/sdbimprt/
      var testCaseDir = TESTCASEDIR + '/';
   }
   println( "testCaseDir = " + testCaseDir + ", or definetion TESTCASEDIR" );
   return testCaseDir;
}

/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj, message )
{
   if( message == undefined ) { message = ""; }
   println( "\n---Begin to create CL " + message + "." );

   if( optionObj == undefined ) { optionObj = { ReplSize: 0 }; }

   commDropCL( db, csName, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCLByOption( db, csName, clName, optionObj, true, true,
      "Failed to create CL." )
   return cl;
}

/* ****************************************************
@description: drop cl
**************************************************** */
function cleanCL ( csName, clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, csName, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

/* ****************************************************
@description: get dataRG Info
@parameter:
   [nameStr] "GroupName","HostName","svcname"
@return: groupArray
**************************************************** */
function getDataGroupsName ()
{
   var tmpArray = commGetGroups( db );
   var groupNameArray = new Array;
   for( i = 0; i < tmpArray.length; i++ )
   {
      groupNameArray.push( tmpArray[i][0].GroupName );
   }
   return groupNameArray;
}

/* ****************************************************
@description: ready tmp director
**************************************************** */
function readyTmpDir ()
{
   println( "\n---Begin to ready tmpFileDir." );

   try
   {
      cmd.run( "rm -rf " + tmpFileDir );
   }
   catch( e )
   {
      println( "Failed to rm tmpFileDir[" + tmpFileDir + "]" );
      throw e;
   }

   try
   {
      cmd.run( "mkdir -p " + tmpFileDir );
   }
   catch( e )
   {
      println( "Failed to mkdir tmpFileDir[" + tmpFileDir + "]" );
      throw e;
   }
}

/* ****************************************************
@description: new Cmd
@return: cmd
**************************************************** */
function cmdInit ()
{
   try
   {
      var cmd = new Cmd();
      return cmd;
   }
   catch( e )
   {
      println( "Failed to init cmd." );
      throw e;
   }
}

/* ****************************************************
@description: get install_dir of sequoiadb
@return: install_dir
**************************************************** */
function getInstallDir ()
{
   try
   {
      var localPath = cmd.run( "pwd" ).split( "\n" )[0] + "/";
      println( "localPath   = " + localPath );

      try
      {
         //get sequoiadb, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb' );
         //get sequoiadb install_dir configurature item, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR"' );
         //get sequoiadb director, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );
         var installPath = tmpDir.split( "\n" )[0] + "/";
      }
      catch( e )
      {
         ///etc/default/sequoiadb is not exists 
         var installPath = localPath;
         println( "instatllpath = " + installPath );
      }
      println( "instatllpath = " + installPath );

   }
   catch( e )
   {
      println( "failed to get global variable : cmd/localPath/installPath" + e );
      throw e;
   }

   return installPath;
}

/* ****************************************************
@description: new File
@return: file
**************************************************** */
function fileInit ( fileName )
{
   try
   {
      var file = new File( fileName );
      return file;
   }
   catch( e )
   {
      println( "Failed to init file." );
      throw e;
   }
}

/* ****************************************************
@description: turn to local time
@parameter:
   time: Timestamp with time zone to millisecond,eg:'1901-12-31T15:54:03.000Z'
   format: eg:%Y-%m-%d-%H.%M.%S.000000
@return: 
   localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   try
   {
      var msecond = new Date( time ).getTime();
      var second = parseInt( msecond / 1000 );  //millisecond to second
      var localtime = cmd.run( 'date -d@"' + second + '" "+' + format + '"' ).split( "\n" )[0];

      return localtime;
   }
   catch( e )
   {
      println( "Timestamp with time zone to local time failed." );
      throw e;
   }
}

function importData ( csName, clName, importFile, type, fields, cast )
{
   println( "\n---Begin to import data." );
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --file ' + importFile;
   +' --insertnum ' + 10000;
   if( type == 'csv' ) 
   {
      imprtOption = imprtOption + ' --fields "' + fields + '"';
   }
   if( cast == true )
   {
      imprtOption = imprtOption + ' --cast ' + cast;
   }
   println( imprtOption );
   /*
   var command = "cat "+ importFile;
   var fileInfo = cmd.run( command );
   println( "\n" + command +"\n" + fileInfo );
   */
   var rc = cmd.run( imprtOption );
   println( rc );
   var rcResults = rc.split( "\n" );

   return rcResults;
}

function exportData ( csName, clName, exportFile, type, fields, sort, otherParam )
{
   println( "\n---Begin to export data." );
   if( typeof ( sort ) == "undefined" ) { sort = "{a:1}"; }

   //remove export file
   cmd.run( "rm -rf " + exportFile );

   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --fields "' + fields + '"'
      + ' --sort "' + sort + '"'
      + ' --file ' + exportFile
      + ' ' + otherParam;
   println( exportOption );
   var rc = cmd.run( exportOption );
   println( rc );

   //cat exprt file
   var command = "cat " + exportFile;
   println( command );
   /*
   var fileInfo = cmd.run( command );
   println( fileInfo ); 
   */
}

function checkImportRC ( rcResults, expParseRecordsNum, expImportedRecordsNum, expParseFailureNum )
{
   println( "\n---Begin to check import results." );
   if( typeof ( expParseFailureNum ) === "undefined" ) { expParseFailureNum = 0; }
   if( typeof ( expImportedRecordsNum ) === "undefined" ) { expImportedRecordsNum = expParseRecordsNum; }

   var expParseRecords = "parsed records: " + expParseRecordsNum;
   var expParseFailure = "parse failure: " + expParseFailureNum;
   var expImportedRecords = "imported records: " + expImportedRecordsNum;
   var actParseRecords = rcResults[0];
   var actParseFailure = rcResults[1];
   var actImportedRecords = rcResults[4];
   if( expParseRecords !== actParseRecords
      || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]",
         "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( cl, expRecsNum, expRecs, cond, message )
{
   if( typeof ( message ) === "undefined" ) 
   {
      message = "";
   }
   else
   {
      message = ", find type: " + message;
   }
   println( "\n---Begin to check cl data" + message + "." );

   var rc;
   if( cond == "undefined" )
   {
      rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   }
   else 
   {
      rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );
   }

   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   // check count
   var actCnt = recsArray.length;
   if( actCnt !== expRecsNum )
   {
      throw buildException( "checkCLdata", null, "[count]", expRecsNum, actCnt );
   }

   // check records
   var actRecs = JSON.stringify( recsArray );
   if( actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[records]", expRecs, actRecs );
   }
}

function checkExportData ( exportFile, expData )
{
   println( "\n---Begin to check export data." );

   var rcData = cmd.run( "cat " + exportFile ).split( "\n" );
   var actData = JSON.stringify( rcData );

   if( actData !== expData )
   {
      throw buildException( "checkCLdata", null, "[export]",
         "[" + expData + "]",
         "[" + actData + "]" );
   }
}

function checkResult ( cl, dataType, expResult )
{
   println( "\n---Begin to check " + dataType + " results." );
   var rc = cl.find( { a: { "$type": 2, "$et": dataType } } ).sort( { _id: 1 } );
   var actResult = [];
   while( rc.next() )
   {
      actResult.push( rc.current().toObj() );
   }

   /*
   for(var i=0;i<10;i++){println("actResult: "+ JSON.stringify(actResult[i]))};
   for(var i=0;i<10;i++){println("expResult:" + JSON.stringify(expResult[i]))};
   */

   if( actResult.length !== expResult.length )
   {
      throw buildException( "checkCLdata", null, "[check length]",
         "[" + expResult.length + "]",
         "[" + actResult.length + "]" );
   }

   for( var i in actResult )
   {
      if( JSON.stringify( actResult[i]['a'] ) !== JSON.stringify( expResult[i]['a'] ) )
      {
         throw buildException( "checkCLdata", null, "[check records, _id = " + JSON.stringify( actResult[i]['_id'] ) + "]",
            "[" + JSON.stringify( expResult[i]['a'] ) + "]",
            "[" + JSON.stringify( actResult[i]['a'] ) + "]" );
      }
   }
}
