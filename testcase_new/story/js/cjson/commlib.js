/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2016/7/28  wu yan Init
*******************************************************************************/
var tmpFileDir = '/tmp/cjsonOfSdbimprt/';
var cmd = cmdInit();
//var installDir  = getInstallDir();   //eg:/opt/sequoiadb
readyTmpDir();
var LocalPath = null;           // 当前目录       
var installDir = initPath();    // import工具所在目录

/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj, message )
{

   if( optionObj == undefined ) { optionObj = { ReplSize: 0 }; }
   if( message == undefined ) { message = ""; }

   println( "\n---Begin to create CL " + message + "." );

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
   commDropCL( db, csName, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

/* ****************************************************
@description: create the tmpDir
**************************************************** */
function readyTmpDir ()
{
   println( "\n---Begin to ready tmpFileDir" );

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

/******************************************************************************
*@Description : initalize the global variable in the begninning.
                初始化全局变量LocalPath、installPath,获取sdbimprt所在目录
******************************************************************************/
function initPath ()
{
   try
   {
      var local = cmd.run( "pwd" ).split( "\n" );   //获得当前目录,cmd.run()方法返回结果会在后面加入一空行
      LocalPath = local[0];
      //println("LocalPath="+LocalPath);
      try
      {
         // 命令返回结果为 INSTALL_DIR=/opt/sequoiadb(默认安装目录)
         // var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );       
         //var installPath = tmpDir.split( "\n" )[0] +"/";
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb' );
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR"' );
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );
         var installPath = tmpDir.split( "\n" )[0] + "/";
      }
      catch( e )
      {
         //找不到安装目录返回当前目录 
         installPath = LocalPath + "/";
         println( "catch erro in try" );
      }


   }
   catch( e )
   {
      println( "instatllpath=" + installPath );
      println( "failed to get global variable : cmd/LocalPath/installPath" + e );
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

/****************************************************
@description: check the result of import
@modify list:
              2016-7-22 yan WU init
****************************************************/
function checkCLData ( cl, expRecs )
{
   println( "\n---Begin to check cl data." );

   var rc = cl.find( {}, { _id: { $include: 0 } } );

   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   //var expRecs = '[{"a":1},{"a":2}]';
   var actRecs = JSON.stringify( recsArray );
   if( actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[expRecs:" + expRecs + "]",
         "[actRecs:" + actRecs + "]" );
   }
   //println( "cl records: "+ actRecs );  
}

/******************************************************
@description: ready the datas of json file ,then import
@modify list:
              2016-7-22 yan WU init
******************************************************/
function importData ( csName, clName, imprtFile, datas )
{
   println( "\n---Begin to ready data." );
   var file = fileInit( imprtFile );
   file.write( datas );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();

   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + imprtFile;
   //println( imprtOption );
   var rc = cmd.run( imprtOption );
   //println( rc );  
   return rc;
}

/******************************************************
@description: remove the removeTmpDir
@modify list:
              2016-7-22 yan WU init
******************************************************/
function removeTmpDir ()
{
   try
   {
      cmd.run( "rm -rf " + tmpFileDir );
   }
   catch( e )
   {
      println( "Failed to remove tmpFileDir[" + tmpFileDir + "]" );
      throw buildException( "removeTmpDir()", e,
         "remove tmpFileDir", "success", "Failed to remove tmpFileDir[" + tmpFileDir + "]" );
   }
}

/******************************************************
@description: check the errorLogMessage of the sdbimport.log
@modify list:
              2016-7-22 yan WU init
******************************************************/
function checkSdbimportLog ( matchInfos, expInfo )
{
   var logInfo = "";
   var result = cmd.run( matchInfos ).split( "\n" )[0];
   var check = result.split( ":" );
   if( check[0] === './sdbimport.log' )
   {
      var length = check.length;
      for( var i = 1; i < length; ++i )
      {
         if( i > 1 )
         {
            logInfo += ":";
         }
         logInfo += check[i];
      }
   }
   else
   {
      logInfo = result;
   }
   //println( "sdbimport.log: "+ logInfo );

   var actLogInfo = logInfo;
   if( expInfo !== actLogInfo )
   {
      throw buildException( "checkSdbimportLog()", null, "[sdbimprt results]",
         "[" + expInfo + "]",
         "[" + actLogInfo + "]" );
   }
}

/******************************************************
@description: check the sdbimport return Infos ,check the success and fail counts
@modify list:
              2016-7-22 yan WU init
******************************************************/
function checkImportReturn ( rc, parseFail, importRes )
{
   var rcObj = rc.split( "\n" );
   var expError = "parse failure: " + parseFail;
   var expImportedRecords = "imported records: " + importRes;
   var actError = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw buildException( "importData", null, "[sdbimprt results]",
         "[" + expError + ", " + expImportedRecords + "]",
         "[" + actError + ", " + actImportedRecords + "]" );
   }
}
