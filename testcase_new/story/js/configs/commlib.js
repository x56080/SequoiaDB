/*******************************************************************************
 * @Description :  common function for test node configs
 * @author      :  Liang XueWang               
 *******************************************************************************/
function ConfDesp ( name, type, defVal, validVal, invalidVal )
{
   this.name = name;
   this.type = type;
   this.defVal = defVal;
   this.validVal = validVal;
   this.invalidVal = invalidVal;
}

ConfDesp.prototype.toString = function()
{
   return ( "conf: " + this.name + " type: " + this.type +
      " default value: " + this.defVal );
}

var Configs = ( function()
{
   var instance;
   var Configs = function()
   {
      if( instance !== undefined ) return instance;
      this.runConfigs = [];
      this.rebootConfigs = [];
      this.forbidConfigs = [];
      this.unknowConfigs = [];
      this.init();
      return instance = this;
   };

   Configs.prototype.init = function()
   {
      // register run configs
      this.runConfigs.push( new ConfDesp( "diagnum", "int", 20, 10, "abc" ) );
      this.runConfigs.push( new ConfDesp( "auditnum", "int", 20, 10, "abc" ) );
      this.runConfigs.push( new ConfDesp( "maxpool", "int", 50, 100, "123" ) );
      this.runConfigs.push( new ConfDesp( "diaglevel", "short", 3, 5, "abc" ) );
      this.runConfigs.push( new ConfDesp( "auditmask", "string", "SYSTEM|DDL|DCL", "SYSTEM", 122 ) );
      this.runConfigs.push( new ConfDesp( "transactiontimeout", "int", 60, 100, "23" ) );
      // this.runConfigs.push( new ConfDesp( "maxsubquery", "int", 10, 0, "abc" ) ) ;  TODO: default actually <= maxprefpool, 0
      this.runConfigs.push( new ConfDesp( "maxreplsync", "int", 10, 12, "abc" ) );
      this.runConfigs.push( new ConfDesp( "sortbuf", "int", 256, 128, null ) );
      this.runConfigs.push( new ConfDesp( "hjbuf", "int", 128, 256, "abc" ) );
      this.runConfigs.push( new ConfDesp( "syncstrategy", "string", "KeepNormal", "KeepAll", 1111 ) );
      this.runConfigs.push( new ConfDesp( "sharingbreak", "int", 7000, 10000, "xxxx" ) );
      this.runConfigs.push( new ConfDesp( "indexscanstep", "int", 100, 200, "what" ) );
      this.runConfigs.push( new ConfDesp( "startshifttime", "int", 600, 1200, "some" ) );
      this.runConfigs.push( new ConfDesp( "preferedinstance", "string", "M", "S", 123 ) );
      this.runConfigs.push( new ConfDesp( "preferedinstancemode", "string", "random", "ordered", 123 ) );
      //this.runConfigs.push( new ConfDesp( "directioinlob", "bool", "FALSE", "TRUE", "1234" ) ) ;
      //this.runConfigs.push( new ConfDesp( "sparsefile", "bool", "FALSE", "TRUE", null ) ) ;
      this.runConfigs.push( new ConfDesp( "weight", "int", 10, 20, "asdafd" ) );
      //this.runConfigs.push( new ConfDesp( "usessl", "bool", "FALSE", "TRUE", "1234" ) ) ;
      //this.runConfigs.push( new ConfDesp( "auth", "bool", "TRUE", "FALSE", "1234" ) ) ;
      this.runConfigs.push( new ConfDesp( "planbuckets", "int", 500, 200, "125" ) );
      this.runConfigs.push( new ConfDesp( "optimeout", "int", 60000, 100000, "20" ) );
      this.runConfigs.push( new ConfDesp( "overflowratio", "int", 12, 22, "11" ) );
      this.runConfigs.push( new ConfDesp( "extendthreshold", "int", 32, 64, "22" ) );
      this.runConfigs.push( new ConfDesp( "signalinterval", "int", 0, 20, "20" ) );
      this.runConfigs.push( new ConfDesp( "maxcachesize", "int", 0, 128, "xxxx" ) );
      this.runConfigs.push( new ConfDesp( "maxcachejob", "int", 10, 20, "####" ) );
      this.runConfigs.push( new ConfDesp( "cachemergesz", "int", 0, 20, "sdfff" ) );
      this.runConfigs.push( new ConfDesp( "pagealloctimeout", "int", 0, 125, "fafrrr" ) );
      this.runConfigs.push( new ConfDesp( "maxsyncjob", "int", 10, 20, "gghttr" ) );
      this.runConfigs.push( new ConfDesp( "syncinterval", "int", 10000, 20000, "sadafe" ) );
      this.runConfigs.push( new ConfDesp( "syncrecordnum", "int", 0, 1000, "ssss" ) );
      //this.runConfigs.push( new ConfDesp( "syncdeep", "bool", "FALSE", "TRUE", "1234" ) ) ;
      //this.runConfigs.push( new ConfDesp( "archivecompresson", "bool", "TRUE", "FALSE", "1234" ) ) ;
      this.runConfigs.push( new ConfDesp( "archivetimeout", "int", 600, 300, "1213" ) );
      this.runConfigs.push( new ConfDesp( "archiveexpired", "int", 240, 120, "no" ) );
      this.runConfigs.push( new ConfDesp( "archivequota", "int", 10, 20, "why" ) );
      this.runConfigs.push( new ConfDesp( "dataerrorop", "int", 1, 2, "I" ) );
      this.runConfigs.push( new ConfDesp( "dmschkinterval", "int", 0, 120, "O" ) );
      //this.runConfigs.push( new ConfDesp( "perfstat", "bool", "FALSE", "TRUE", "1234" ) ) ;
      this.runConfigs.push( new ConfDesp( "optcostthreshold", "int", 20, 10, "M" ) );
      this.runConfigs.push( new ConfDesp( "maxconn", "int", 0, 3000, "12345" ) );
      //this.runConfigs.push( new ConfDesp( "enablemixcmp", "bool", "FALSE", "TRUE", "1234" ) ) ;
      this.runConfigs.push( new ConfDesp( "plancachelevel", "int", 3, 4, "88" ) );
      this.runConfigs.push( new ConfDesp( "memdebug", "bool", "FALSE", "TRUE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "memdebugsize", "int", 0, 256, "WEYEH" ) );
      this.runConfigs.push( new ConfDesp( "memdebugverify", "bool", "FALSE", "TRUE", "1234" ) );

      // register reboot configs
      // this.rebootConfigs.push( new ConfDesp( "diagpath", "path", "", "", null ) ) ; TODO: path can't specific
      // this.rebootConfigs.push( new ConfDesp( "auditpath", "path", "", "", null ) ) ;
      this.rebootConfigs.push( new ConfDesp( "transactionon", "bool", "TRUE", "FALSE", "1234" ) );
      this.rebootConfigs.push( new ConfDesp( "numpreload", "int", 0, 10, "and" ) );
      // this.rebootConfigs.push( new ConfDesp( "maxprefpool", "int", 0, 10, "or" ) ) ; TODO: affect maxsubquery
      this.rebootConfigs.push( new ConfDesp( "logbuffsize", "int", 1024, 2048, "Q" ) );
      // this.rebootConfigs.push( new ConfDesp( "tmppath", "path", "", "", null ) ) ;
      this.rebootConfigs.push( new ConfDesp( "replbucketsize", "int", 32, 64, "test" ) );
      this.rebootConfigs.push( new ConfDesp( "dpslocal", "bool", "FALSE", "TRUE", "1234" ) );
      this.rebootConfigs.push( new ConfDesp( "traceon", "bool", "FALSE", "TRUE", 12345 ) );
      this.rebootConfigs.push( new ConfDesp( "tracebufsz", "int", 256, 512, "lxw" ) );
      this.rebootConfigs.push( new ConfDesp( "archiveon", "bool", "FALSE", "TRUE", 54321 ) );
      // this.rebootConfigs.push( new ConfDesp( "bkuppath", "path", "", "", null ) ) ;
      this.rebootConfigs.push( new ConfDesp( "instanceid", "int", 0, 1, "aassada" ) );

      // register forbid configs
      this.forbidConfigs.push( new ConfDesp( "dbpath", "path", "./", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "indexpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "confpath", "path", "./", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "logpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "wwwpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "svcname", "string", "11810", "50000", null ) );
      this.forbidConfigs.push( new ConfDesp( "replname", "string", "", "51000", null ) );
      this.forbidConfigs.push( new ConfDesp( "shardname", "string", "", "52000", null ) );
      this.forbidConfigs.push( new ConfDesp( "catalogname", "string", "", "53000", null ) );
      this.forbidConfigs.push( new ConfDesp( "httpname", "string", "", "54000", null ) );
      this.forbidConfigs.push( new ConfDesp( "omname", "string", "", "55000", null ) );
      this.forbidConfigs.push( new ConfDesp( "role", "string", "standalone", "data", "person" ) );
      this.forbidConfigs.push( new ConfDesp( "catalogaddr", "string", "", "hdgfdkj:57000", null ) );
      this.forbidConfigs.push( new ConfDesp( "logfilesz", "int", 64, 128, "10" ) );
      this.forbidConfigs.push( new ConfDesp( "logfilenum", "int", 20, 10, "128" ) );
      this.forbidConfigs.push( new ConfDesp( "lobpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "lobmetapath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "omaddr", "string", "", "helloworld:54345", null ) );
      this.forbidConfigs.push( new ConfDesp( "archivepath", "path", "", "helloworld", null ) );

      // register unknown configs
      this.unknowConfigs.push( new ConfDesp( "cataloglist", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "clustername", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "businessname", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "usertag", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "fap", "string", "fapmongo", "fapmongo", null ) );
      this.unknowConfigs.push( new ConfDesp( "arbiter", "bool", "FALSE", "TRUE", true ) );
   };
   return Configs;
} )();

function getRandomRunConf ()
{
   var configs = new Configs();
   var len = configs.runConfigs.length;
   var idx = getRandomIdx( 0, len );
   return configs.runConfigs[idx];
}

function getRandomRebootConf ()
{
   var configs = new Configs();
   var len = configs.rebootConfigs.length;
   var idx = getRandomIdx( 0, len );
   return configs.rebootConfigs[idx];
}

function getRandomForbidConf ()
{
   var configs = new Configs();
   var len = configs.forbidConfigs.length;
   var idx = getRandomIdx( 0, len );
   return configs.forbidConfigs[idx];
}

function getAllRunConf ()
{
   var configs = new Configs();
   return configs.runConfigs;
}

function getAllRebootConf ()
{
   var configs = new Configs();
   return configs.rebootConfigs;
}

function getAllForbidConf ()
{
   var configs = new Configs();
   return configs.forbidConfigs;
}

/************************************************************************
 * @Description : get random number in [ low, high )
 * @author      : Liang XueWang
 ************************************************************************/
function getRandomIdx ( low, high )
{
   var range = high - low;
   return parseInt( Math.random() * range + low );
}

/************************************************************************
 * @Description : get all data groups in cluster
 *                return array like [ "group1", "group2", ... ]
 *                if standalone, return empty array []
 * @author      : Liang XueWang
 ************************************************************************/
function getDataGroups ( db )
{
   var groups = [];
   if( commIsStandalone( db ) )
   {
      return groups;
   }
   var cursor = db.listReplicaGroups();
   var tmpInfo;
   while( tmpInfo = cursor.next() )
   {
      var groupName = tmpInfo.toObj().GroupName;
      if( groupName == "SYSCoord" || groupName == "SYSCatalogGroup" )
         continue;
      groups.push( groupName );
   }
   return groups;
}

/******************************************************************
 * @Description : get nodes in group
 *                rgName: group name
 *                return nodes array, like [ "ci-test-pm2:20100", .... ]
 *                if standalone, return empty array
 * @author      : Liang XueWang
 ******************************************************************/
function getGroupNodes ( db, rgName )
{
   var nodes = [];
   if( commIsStandalone( db ) )
   {
      return nodes;
   }
   var tmpObj = db.getRG( rgName ).getDetail().next().toObj();
   var tmpGroupArray = tmpObj["Group"];
   for( var i = 0; i < tmpGroupArray.length; i++ )
   {
      var tmpNodeObj = tmpGroupArray[i];
      var nodename = tmpNodeObj["HostName"];
      for( var j = 0; j < tmpNodeObj.Service.length; j++ )
      {
         var tmpSvcObj = tmpNodeObj.Service[j];
         if( tmpSvcObj["Type"] == 0 )
         {
            nodename = nodename + ":" + tmpSvcObj["Name"];
            nodes.push( nodename );
            break;
         }
      }
   }

   return nodes;
}

/************************************************************************
 * @Description : get local hostname like "sdbserver1"
 * @author      : Liang XueWang
 ************************************************************************/
function getLocalHostName ()
{
   return System.getHostName();
}

/************************************************************************
 * @Description : create and start node in group
 *                rg: group handle
 *                host: node hostname
 *                svc: node svcname
 *                path: node dbpath
 *                return srcLogPath: the created node log path
 * @author      : Liang XueWang
 ************************************************************************/
function createAndStartNode ( rg, host, svc, path )
{
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   var svcNameAndsrcLogPath = null;
   do
   {
      try
      {
         var node = rg.createNode( host, svc, path, { diaglevel: 5 } );
         println( "create node: " + host + ":" + svc + " " + path );
         checkSucc = true;
      }
      catch( e )
      {
         //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
         if( e == -145 || e == -290 )
         {
            svc = parseInt( svc ) + 10;
            path = RSRVNODEDIR + "data/" + svc;
         }
         else
         {
            throw "create node failed!  port = " + svc + " path = " + path + " errorCode: " + e;
         }
         times++;
      }
   }
   while( !checkSucc && times < maxRetryTimes );
   println( "start node" );
   node.start();
   var srcLogPath = host + ":" + CMSVCNAME + "@" + path + "/diaglog/sdbdiag.log";
   svcNameAndsrcLogPath = { "svcName": svc, "srcLogPath": srcLogPath };
   return svcNameAndsrcLogPath;
}

/************************************************************************
 * @Description : remove node in group
 *                rg: group handle
 *                host: node hostname
 *                svc: node svcname
 * @author      : Liang XueWang
 ************************************************************************/
function removeNode ( rg, host, svc )
{
   try
   {
      println( "remove node: " + host + ":" + svc );
      rg.removeNode( host, svc );
   }
   catch( e )
   {
      throw buildException( "removeNode", e, "remove node", 0, e );
   }
}

/************************************************************************
 * @Description : create group and start
 *                db: connection handle, can't be standalone
 *                rgName: group name
 *                nodesNum: node num, node svc like 26000 26010 ....
 *
 *                return logSourcePaths: log paths to be backed up
 * @author      : Liang XueWang
 ************************************************************************/
function createAndStartGroup ( db, rgName, nodesNum )
{
   var rg = db.createRG( rgName );
   var host = getLocalHostName();
   var failedCount = 0;
   var logSourcePaths = [];
   for( var i = 0; i < nodesNum; i++ )
   {
      var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbPath = RSRVNODEDIR + "data/" + svc;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      do
      {
         try
         {
            rg.createNode( host, svc, dbPath, { diaglevel: 5 } );
            println( "create node: " + host + ":" + svc + " dbpath: " + dbPath );
            checkSucc = true;
            logSourcePaths.push( host + ":" + CMSVCNAME + "@" + dbPath + "/diaglog/sdbdiag.log" );
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == -145 || e == -290 )
            {
               svc = svc + 10;
               dbPath = RSRVNODEDIR + "data/" + svc;
               failedCount++;
            }
            else
            {
               throw "create node failed!  port = " + svc + " dataPath = " + dbPath + " errorCode: " + e;
            }
            times++;
         }
      }
      while( !checkSucc && times < maxRetryTimes );
   }
   println( "start group" );
   rg.start();
   return logSourcePaths;
}

/************************************************************************
 * @Description : remove group
 *                db: connection handle, can't be standalone
 *                rgName: group name
 * @author      : Liang XueWang
 ************************************************************************/
function removeGroup ( db, rgName )
{
   try
   {
      db.removeRG( rgName );
   }
   catch( e )
   {
      throw buildException( "removeGroup", e, "remove group " + rgName, 0, e );
   }
}

/************************************************************************
 * @Description : update config
 *                db: connection handle
 *                config: update config object
 *                option: update option object
 *                errno: errno expected
 * @author      : Liang XueWang
 ************************************************************************/
function updateConf ( db, config, option, errno )
{
   try
   {
      db.updateConf( config, option );
      if( errno !== undefined ) throw 0;
   }
   catch( e )
   {
      if( errno === undefined )
      {
         throw buildException( "updateConf", e, "update conf with config: " + JSON.stringify( config ) +
            " and option: " + JSON.stringify( option ), 0, e );
      }
      else if( e !== errno )
      {
         throw buildException( "updateConf", e, "update conf with config: " + JSON.stringify( config ) +
            " and option: " + JSON.stringify( option ), errno, e );
      }
   }
}

/************************************************************************
 * @Description : delete config
 *                db: connection handle
 *                config: delete config object
 *                option: delete option object
 *                errno: errno expected
 * @author      : Liang XueWang
 ************************************************************************/
function deleteConf ( db, config, option, errno )
{
   try
   {
      db.deleteConf( config, option );
      if( errno !== undefined ) throw 0;
   }
   catch( e )
   {
      if( errno === undefined )
      {
         throw buildException( "deleteConf", e, "delete conf with config: " + JSON.stringify( config ) +
            " and option: " + JSON.stringify( option ), 0, e );
      }
      else if( e !== errno )
      {
         throw buildException( "deleteConf", e, "delete conf with config: " + JSON.stringify( config ) +
            " and option: " + JSON.stringify( option ), errno, e );
      }
   }
}

/************************************************************************
 * @Description : snapshot config on node
 *                host: node hostname
 *                svc: node svcname
 *                return conf obj, like { dbpath: "xxx", .... }
 * @author      : Liang XueWang
 ************************************************************************/
function getConfFromSnapshot ( host, svc )
{
   try
   {
      var conn = new Sdb( host, svc );
      var cursor = conn.snapshot( SDB_SNAP_CONFIGS );
      var obj = cursor.next().toObj();
   }
   catch( e )
   {
      throw buildException( "getConfFromSnapshot", e, "snapshot conf of node: " +
         host + ":" + svc, 0, e );
   }
   if( cursor.next() !== undefined )
   {
      throw buildException( "getConfFromSnapshot", null, "snapshot conf of node " +
         host + ":" + svc + " reurn two result", 1, 2 );
   }
   conn.close();
   return obj;
}

/************************************************************************
 * @Description : get node conf from conf file
 *                host: node hostname
 *                svc: node svcname
 *                return conf obj, like { dbpath: "xxx", .... }
 * @author      : Liang XueWang
 ************************************************************************/
function getConfFromFile ( host, svc )
{
   var confFile = commGetInstallPath() + "/conf/local/" + svc + "/sdb.conf";
   var remote = new Remote( host, CMSVCNAME );
   var cmd = remote.getCmd();
   var obj = {};
   try
   {
      var arr = cmd.run( "cat " + confFile ).split( "\n" );
      for( var i = 0; i < arr.length - 1; i++ )
      {
         var info = arr[i].split( "=" );
         var key = info[0];
         var val = info[1];
         obj[key] = val;
      }
      remote.close();
   }
   catch( e )
   {
      throw buildException( "getConfFromFile", e, "get node: " + host + ":" + svc +
         " from file " + confFile, 0, e );
   }
   return obj;
}

/************************************************************************
 * @Description : check snapshot conf between before and after
 *                before: snapshot info before
 *                after: snapshot info after
 *                confs: confs which should be changed
 * @author      : Liang XueWang
 ************************************************************************/
function checkSnapshot ( before, after, confs )
{
   var num1 = Object.getOwnPropertyNames( before ).length;
   var num2 = Object.getOwnPropertyNames( after ).length;
   if( num1 !== num2 )
   {
      throw buildException( "checkSnapshot", null, "check conf num equal",
         num1, num2 );
   }
   for( var key in before )
   {
      if( confs === undefined || confs[key] === undefined )
      {
         if( after[key] !== before[key] )
         {
            println( "check conf " + key + "," + before[key] + "," + after[key] + "," +
               typeof ( after[key] ) );
            if( typeof ( after[key] ) !== "string" )
            {
               throw buildException( "checkSnapshot", null, "check conf " + key,
                  before[key], after[key] );
            }
            else   // path
            {
               var max = ( after[key].length > before[key].length ) ? after[key] : before[key];
               var min = ( max === after[key] ) ? before[key] : after[key];
               if( max.indexOf( min ) === -1 )
               {
                  throw buildException( "checkSnapshot", null, "check path conf " + key,
                     before[key], after[key] );
               }
            }
         }
      }
      else
      {
         if( after[key] !== confs[key] )
         {
            println( "check conf " + key );
            throw buildException( "checkSnapshot", null, "check changed conf " + key,
               confs[key], after[key] );
         }
      }
   }
}

/************************************************************************
 * @Description : check conf file between before and after
 *                before: file info before
 *                after: file info after
 *                confs: confs which should be changed
 * @author      : Liang XueWang
 ************************************************************************/
function checkConfFile ( before, after, confs )
{
   for( var key in after )
   {
      if( confs !== undefined && confs[key] !== undefined )
      {
         if( after[key] != confs[key] )  // type may not be the same
         {
            println( "check conf " + key + "," + confs[key] + "," + after[key] );
            throw buildException( "checkConfFile", null, "check change conf " + key,
               confs[key], after[key] );
         }
      }
      else if( before[key] === undefined || after[key] === undefined )
      {
         // println( "No conf " + key + " in file before or after" ) ;
         continue;
      }
      else
      {
         if( after[key] !== before[key] &&
            after[key].indexOf( before[key] ) === -1 )   // path
         {
            println( "check conf " + key + " before: " + before[key] + " after: " + after[key] );
            throw buildException( "checkConfFile", null, "check conf " + key,
               before[key], after[key] );
         }
      }
   }
}
