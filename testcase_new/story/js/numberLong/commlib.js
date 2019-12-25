/*******************************************************************************
@Description : common functions
@Modify list : 2016-3-28  Ting YU  Init
*******************************************************************************/

import( "../lib/basic_operation/sequoiadb.js" );

function checkExplain ( rc, expIdxName )
{
   var plan = rc.explain().current().toObj();
   var expScanType = "ixscan";
   if( expIdxName === undefined ) { expScanType = "tbscan"; }

   if( plan.ScanType !== expScanType )
   {
      throw new Error( "plan.ScanType: " + plan.ScanType + "\nexpScanType: " + expScanType );
   }
   if( plan.IndexName !== expIdxName )
   {
      throw new Error( "plan.IndexName: " + plan.IndexName + "\nexpIdxName: " + expIdxName );
   }
}

function select2RG ()
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
function runCurl ( curlPara, expErrno )
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
function geneCurlCommad ( curlPara )
{
   //head of curl command
   var restPort = parseInt( COORDSVCNAME, 10 ) + 4;
   var curlHead = 'curl http://' + COORDHOSTNAME + ':' + restPort + '/ -d';

   //main of curl command
   var curlMain = "'"; //begin with ', not with "
   for( var i in curlPara )
   {
      curlMain += curlPara[i];
      curlMain += '&';
   }
   curlMain = curlMain.substring( 0, curlMain.length - 1 ); //remove last character &
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
function resolveRtnInfo ( rtnInfo )
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
