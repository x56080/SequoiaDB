import( "../lib/main.js" );
import( "../lib/lobSubCL_commlib.js" );

var srcCSName = "comm_srcCS";
var srcCLName = "comm_srcCL";
var srcDataName = "srcData";

var datasrcIp = DSHOSTNAME;
// CI暂时只提供一台主机
// var datasrcIp1 = "192.168.31.39";
// 部分用例数据源需要多个集群，CI不运行，本地运行时自行配置
// var other_datasrcIp1 = "192.168.31.41";
// var other_datasrcIp2 = "192.168.20.44";
var datasrcPort = DSSVCNAME;
var userName = "sdbadmin";
var passwd = "sdbadmin";
var datasrcUrl = datasrcIp + ":" + datasrcPort;
// var datasrcUrl1 = datasrcIp1 + ":" + datasrcPort;
// var otherDSUrl1 = other_datasrcIp1 + ":" + datasrcPort;
// var otherDSUrl2 = other_datasrcIp2 + ":" + datasrcPort;


var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
// var datasrcDB1 = new Sdb( datasrcIp1, datasrcPort, userName, passwd );

function clearDataSource ( csName, dataSrcName )
{
   try
   {
      db.dropCS( csName );
   }
   catch( e )
   {
      if( e != SDB_DMS_CS_NOTEXIST )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.dropDataSource( dataSrcName )
   }
   catch( e )
   {
      if( e != SDB_CAT_DATASOURCE_NOTEXIST )
      {
         throw new Error( e );
      }
   }
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}



/************************************
*@Description: bulk insert data
*@author:      zhaoyu
*@createDate:  2016.11.23
**************************************/
function insertBulkData ( dbcl, recordNum, recordStart, recordEnd )
{
   try
   {
      var doc = [];
      for( var i = 0; i < recordNum; i++ )
      {
         var bValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         var cValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         doc.push( { a: i, b: bValue, c: cValue } );
      }
      dbcl.insert( doc );
      println( "--bulk insert data success" );
   }
   catch( e )
   {
      throw buildException( "insertBulkData()", e, "insert", "insert data :" + JSON.stringify( doc ), "insert fail" );
   }
   return doc;
}

function getDSMajorVersion ( dataSrcName )
{
   var DSVersion = db.listDataSources( { Name: dataSrcName } ).current().toObj().DSVersion;
   var majorVersion = DSVersion.slice( 0, 1 );
   return majorVersion;
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getRG( 'SYSCoord' ).getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}