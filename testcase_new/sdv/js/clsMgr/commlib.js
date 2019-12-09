/* *****************************************************************************
@Description: clsMgr common functions
@author:      LY
***************************************************************************** */

// common functions
function insertAndCheck ( cl, num, removeAll, needCheck, message )
{
   if( undefined == num ) 
   {
      num = 1;
   }
   if( undefined == message ) 
   {
      message = "";
   }
   if( undefined == needCheck ) 
   {
      needCheck = true;
   }
   if( undefined == removeAll ) 
   {
      removeAll = true;
   }

   // remove cl records first
   if( removeAll )
   {
      try
      {
         cl.remove();
      }
      catch( e )
      {
         println( "Remove records failed: " + e + ", message: " + message );
         throw buildException(
            "cl remove",
            e,
            "cl.remove()",
            "succ",
            "failure"
         );
      }
   }

   // insert records
   try
   {
      for( var i = 0; i < num; ++i )
      {
         cl.insert( { "no": i, "b": 2 * i, "c": false, "d": { "da": [i, i + 1, "oo"], "db": { "dba": "test" } } } );
      }
   }
   catch( e )
   {
      println( "Insert " + i + " records failed: " + e + ", message: " + message );
      throw buildException(
         "cl insert",
         e,
         "cl.insert()",
         "succ",
         "failure"
      );
   }

   // check insert records
   if( !needCheck )
   {
      return;
   }
   var size = 0;
   try
   {
      size = cl.count();
      if( size != num )
      {
         println( "Count all size: " + size + " is not same with " + num + ", message: " + message );
         throw ( "count is not equel, check failed" );
      }
   }
   catch( e )
   {
      println( "Count all exception: " + e + ", message: " + message );
      throw buildException(
         "cl count",
         e,
         "cl.count()",
         num,
         size
      );
   }

   // check insert records by find method
   var rc;
   try
   {
      size = 0;
      rc = cl.find( { "no": { "$gte": 0, "$lt": num } } );
      while( rc.next() )
      {
         ++size;
      }
      if( size !== num )
      {
         println( "Count find size: " + size + " is not same with " + num + ", message: " + message );
         throw ( "count by find method is not equal, check failed" );
      }
   }
   catch( e )
   {
      println( "Count find exception: " + e + ", message: " + message );
      throw buildException(
         "cl find",
         e,
         "cl.find()",
         num,
         size
      );
   }
}

function getHostName ()
{
   var cmd = new Cmd();
   try
   {
      var hostname = cmd.run( "hostname" ).split( /(\n)/ );
      return hostname[0];
   }
   catch( e )
   {
      throw buildException(
         "get hostname",
         e,
         "hostname",
         "succ",
         "failure"
      );
   }
}

function generatePort ( initialNum, message )
{
   if( undefined === initialNum ) 
   {
      initialNum = 20000;
   }
   if( undefined === message ) 
   {
      message = "";
   }

   var num = parseInt( initialNum ) + 10;
   if( paraIsExist( num ) )
   {
      num = generatePort( num );
   }
   var para = num.toString();
   return para;
}

function paraIsExist ( port, message )
{
   if( undefined === message ) 
   {
      message = "";
   }
   if( undefined === port ) 
   {
      throw ( "the test port is null,error" );
   }

   var cmd = new Cmd();
   try
   {
      var result = cmd.run( "lsof -i:" + port );
      if( undefined !== result )
      {
         return true;
      }
   }
   catch( e )
   {
      println( "This port " + port + " is not occupied." );
      return false;
   }
}

function connectSdb ( hostname, svcname, port, isStandalone )
{
   if( undefined === hostname ) 
   {
      hostname = COORDHOSTNAME;
   }
   if( undefined === svcname ) 
   {
      svcname = CMSVCNAME;
   }
   if( undefined === port ) 
   {
      throw ( "the temp crood port is null,error" );
   }
   if( undefined === isStandalone ) 
   {
      isStandalone = true;
   }

   // build temporary coord
   var oma = new Oma( hostname, svcname );
   if( isStandalone )
   {
      try
      {
         oma.createData( port, SPAREPORTPATH + port );
         oma.startNode( port );
      }
      catch( e )
      {
         println( "Enable standalone data failed: " + e );
         throw buildException(
            "build standalone data",
            e,
            "oma.createData( " + port + ", " + SPAREPORTPATH + port + " );oma.startNode( " + port + " )",
            "succ",
            "failure"
         );
      }
   }
   else
   {
      try
      {
         oma.createCoord( port, SPAREPORTPATH + port );
         oma.startNode( port );
      }
      catch( e )
      {
         println( "Enable temporary coord failed: " + e );
         throw buildException(
            "build temporary coord",
            e,
            "oma.createCoord( " + port + ", " + SPAREPORTPATH + port + " );oma.startNode( " + port + " )",
            "succ",
            "failure"
         );
      }
   }

   // common database connection
   try
   {
      var db = new Sdb( hostname, port );
      return db;
   }
   catch( e )
   {
      println( "Connect Failed in connectSdb function!" );
      buildException(
         "connect temp coord",
         e,
         "var db = new Sdb( " + hostname + ", " + port + " )",
         "succ",
         "failure"
      );
   }
}

function sleep ( second )
{
   var time = second * 1000;
   var start = new Date().getTime();
   while( true )
   {
      if( new Date().getTime() - start > time )
         break;
   }
}

function getMasterHostname ( group )
{
   var ret = group.getDetail().current();
   var masterId = ret.toObj()["PrimaryNode"];
   for( var i = 1; i <= ret.toObj()["Group"].length; i++ )
   {
      if( masterId === ret.toObj()["Group"][i]["NodeID"] )
      {
         var masterHostname = ret.toObj()["Group"][i]["HostName"];
         return masterHostname;
      }
   }
}

function getMasterSvcname ( group )
{
   var ret = group.getDetail().current();
   var masterId = ret.toObj()["PrimaryNode"];
   for( var i = 1; i <= ret.toObj()["Group"].length; i++ )
   {
      if( masterId === ret.toObj()["Group"][i]["NodeID"] )
      {
         var masterSvcname = ret.toObj()["Group"][i]["Service"][0]["Name"];
         return masterSvcname;
      }
   }
}

function getSlaverHostname ( group )
{
   var ret = group.getDetail().current();
   var masterId = ret.toObj()["PrimaryNode"];
   for( var i = 1; i <= ret.toObj()["Group"].length; i++ )
   {
      if( masterId !== ret.toObj()["Group"][i]["NodeID"] )
      {
         var slaveHostname = ret.toObj()["Group"][i]["HostName"];
         return slaveHostname;
      }
   }
}

function getSlaverSvcname ( group )
{
   var ret = group.getDetail().current();
   var masterId = ret.toObj()["PrimaryNode"];
   for( var i = 1; i <= ret.toObj()["Group"].length; i++ )
   {
      if( masterId !== ret.toObj()["Group"][i]["NodeID"] )
      {
         var slaveSvcname = ret.toObj()["Group"][i]["Service"][0]["Name"];
         return slaveSvcname;
      }
   }
}

function clean ( db, hostname, port, groupName, type )
{
   // the regular expressions of type
   re = "((?:[a-z][a-z]+)_group)";
   if( undefined === hostname ) 
   {
      hostname = COORDHOSTNAME;
   }
   if( undefined === port && undefined === re.exec( type ) ) 
   {
      throw ( "the clean port is null,error" );
   }
   if( undefined === groupName ) 
   {
      groupName = "group1";
   }
   if( undefined === type ) 
   {
      throw ( "the clean type is null,error" );
   }

   switch( type )
   {
      case "coord":
         db.getCoordRG().removeNode( hostname, port );
         break;
      case "catalog":
         db.getCatalogRG().removeNode( hostname, port );
         break;
      case "data":
         db.getRG( groupName ).removeNode( hostname, port );
         break;
      case "tmp_coord":
         var oma = new Oma( hostname, CMSVCNAME );
         oma.removeCoord( port );
         break;
      case "standalone_data":
         var oma = new Oma( hostname, CMSVCNAME );
         oma.removeData( port );
         break;
      case "catalog_group":
         db.removeCatalogRG();
         break;
      case "data_group":
         db.removeRG( groupName );
         break;
      case "coord_group":
         db.removeCoordRG();
         break;
      default:
         throw ( "there is no such type, please select one of 'crood, catalog, data, tmp_coord, standalone_data, catalog_group, data_group, coord_group'" );
   }
}