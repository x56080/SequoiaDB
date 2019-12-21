/*******************************************************************************
* @Description :  common function for sdbexprt
* @author      :  Liang XueWang
*                
*******************************************************************************/
var cmd = new Cmd();
var installPath = getInstallDir();
var tmpFileDir = WORKDIR + "/sdbexprt/";
var workDir = readyTmpDir();


/* ***************************************************
@description : 获取bin/sdbexprt所在目录
@author: XiaoNi Huang 2019-12-19
**************************************************** */
function getInstallDir ()
{
   var localDir = cmd.run( "pwd" ).split( "\n" )[0] + "/";
   println( "localDir   = " + localDir );
   var installDir = '';

   // 先取当前目录下的 bin/sdbimprt，不存在时，再取安装目录下的 bin/sdbimprt
   try
   {
      cmd.run( 'find ./bin/sdbexprt' ).split( '\n' )[0];
      installDir = localDir;
   }
   catch( e ) 
   {
      installDir = commGetInstallPath() + "/";
   }

   println( "instatllpath = " + installDir );
   return installDir;
}

/* ****************************************************
@description: ready tmp director
@author: XiaoNi Huang 2019-12-19
**************************************************** */
function readyTmpDir ()
{
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

/*******************************************************************
* @Description : check host is localhost or local hostname
* @author      : Liang XueWang
*
********************************************************************/
function isLocal ( hostname )
{
   if( hostname === "localhost" )
      return true;
   if( hostname === cmd.run( "hostname" ).split( "\n" )[0] )
      return true;
   return false;
}

/*******************************************************************
* @Description : get groups, include SYSCoord and SYSCatalogGroup
*                return array like [ "group1", ... ]
* @author      : Liang XueWang
*
********************************************************************/
function getGroups ()
{
   var groups = [];
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone, no groups" );
      return groups;
   }
   var cursor = db.listReplicaGroups();
   var obj;
   while( obj = cursor.next() )
   {
      var groupName = obj.toObj()["GroupName"];
      groups.push( groupName );
   }
   return groups;
}

/*******************************************************************
* @Description : get group nodes, 
*                return array like [ "sdbserver1:11830", ... ]
* @author      : Liang XueWang
*
********************************************************************/
function getGroupNodes ( groupName )
{
   var nodes = [];
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone, no group " + groupName );
      return nodes;
   }
   var rg = db.getRG( groupName );
   var obj = rg.getDetail().next().toObj();
   var groupArr = obj["Group"];
   for( var i = 0; i < groupArr.length; i++ )
   {
      var hostname = groupArr[i]["HostName"];
      var svcname = groupArr[i]["Service"][0]["Name"];
      nodes.push( hostname + ":" + svcname );
   }
   return nodes;
}

/*******************************************************************
* @Description : get a random int [m n)      
* @author      : Liang XueWang
*
********************************************************************/
function getRandomInt ( m, n )
{
   var range = n - m;
   var ret = m + parseInt( Math.random() * range );
   return ret;
}

/*******************************************************************
* @Description : insert record which use kilobytes space       
* @author      : Liang XueWang
*
********************************************************************/
function insertKBDocs ( cl, kb )
{
   var docs = [];

   for( var i = 0; i < parseInt( kb ); i++ )
   {
      var doc = {};
      doc["key"] = makeString( 1024, 'x' );
      doc["cnt"] = i + 1;
      docs.push( doc );
      if( docs.length % 10000 === 0 || i === parseInt( kb ) - 1 )
      {
         cl.insert( docs );
         docs = [];
      }
   }
}

/*******************************************************************
* @Description : test run command        
* @author      : Liang XueWang
*
********************************************************************/
function testRunCommand ( command, errno )
{
   try
   {
      cmd.run( command );
      if( errno !== undefined ) throw 0;
   }
   catch( e )
   {
      if( errno === undefined )
      {
         throw buildException( "testRunCommand", e,
            "run command:\n" + command, 0, e );
      }
      else if( e !== errno )
      {
         throw buildException( "testRunCommand", e,
            "run command:\n" + command, errno, e );
      }
   }
}

/*******************************************************************
* @Description : check file content       
* @author      : Liang XueWang
*
********************************************************************/
function checkFileContent ( filename, expContent )
{
   try
   {
      var size = parseInt( File.stat( filename ).toObj().size );
      var file = new File( filename );
      var actContent = file.read( size );
      file.close();
   }
   catch( e )
   {
      throw buildException( "checkFileContent", null,
         "read " + filename, 0, e );
   }
   if( actContent !== expContent )
   {
      throw buildException( "checkFileContent", null,
         "check " + filename + " content",
         expContent.slice( 0, 1024 ), actContent.slice( 0, 1024 ) );
   }
}

/*******************************************************************
* @Description : get records from cursor, return record array     
* @author      : Liang XueWang
*
********************************************************************/
function getRecords ( cursor )
{
   var recs = [];
   var obj;
   while( obj = cursor.next() )
   {
      recs.push( JSON.stringify( obj.toObj() ) );
   }
   return recs;
}

/*******************************************************************
* @Description : check records       
* @author      : Liang XueWang
*
********************************************************************/
function checkRecords ( expRecs, actRecs )
{
   if( expRecs.length !== actRecs.length )
   {
      throw buildException( "checkRecords", null, "check rec count",
         expRecs.length, actRecs.length );
   }
   for( var i = 0; i < expRecs.length; i++ )
   {
      if( expRecs[i] !== actRecs[i] )
      {
         throw buildException( "checkRecords", null, "check record " + i,
            JSON.stringify( expRecs[i] ).slice( 0, 1024 ),
            JSON.stringify( actRecs[i] ).slice( 0, 1024 ) );
      }
   }
}

/*******************************************************************
* @Description : check file exist  
* @author      : Liang XueWang
*
********************************************************************/
function checkFileExist ( filename, exist )
{
   var res = File.exist( filename );
   if( res !== exist )
   {
      throw buildException( "checkFileExist", null,
         "check " + filename, exist, res );
   }
}

/*******************************************************************
* @Description : get current user
* @author      : Liang XueWang
*
********************************************************************/
function getCurrentUser ()
{
   return System.getCurrentUser().toObj()["user"];
}

/*******************************************************************
* @Description : make string with length len
* @author      : Liang XueWang
*
********************************************************************/
function makeString ( len, ch )
{
   var arr = new Array( len + 1 );
   return arr.join( ch );
}

/*******************************************************************
* @Description : remove rec file if needed
* @author      : Liang XueWang
*
********************************************************************/
function rmRecFile ( csname, clname )
{
   var files = csname + "_" + clname + "_*.rec";
   cmd.run( "rm -rf ./" + files );
}