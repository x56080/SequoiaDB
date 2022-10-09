import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


/*******************************************************************************
@Description : 执行所有查看快照操作
@Modify 
*******************************************************************************/
function snapshotOpr ( sdb )
{
   var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
      SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_CATALOG, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
      SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_SEQUENCES, SDB_SNAP_QUERIES, SDB_SNAP_LOCKWAITS,
      SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      sdb.snapshot( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看快照操作
@Modify 
*******************************************************************************/
function snapshotOprForOtherNode ( sdb )
{
   var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
      SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
      SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_QUERIES,
      SDB_SNAP_LOCKWAITS, SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      sdb.snapshot( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看list操作
@Modify 
*******************************************************************************/
function listOpr ( sdb )
{
   var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS, SDB_LIST_COLLECTIONSPACES,
      SDB_LIST_STORAGEUNITS, SDB_LIST_GROUPS, SDB_LIST_TASKS, SDB_LIST_TRANSACTIONS, SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS, SDB_LIST_SEQUENCES,
      SDB_LIST_USERS, SDB_LIST_BACKUPS, SDB_LIST_DATASOURCES];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      sdb.list( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看list操作
@Modify 
*******************************************************************************/
function listOprForOtherNode ( sdb )
{
   var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS,
      SDB_LIST_COLLECTIONSPACES,SDB_LIST_STORAGEUNITS, SDB_LIST_TRANSACTIONS,SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS, SDB_LIST_BACKUPS];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      sdb.list( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看list操作
@Modify 
*******************************************************************************/
function createAndRemoveNode ( sdb )
{
   var groups = commGetGroups( sdb );
   var groupName = groups[0][0].GroupName;
   var rg = sdb.getRG( groupName );
   var hostName = groups[0][1].HostName;
   var svc = parseInt( RSRVPORTBEGIN ) + 40;
   var dbpath = RSRVNODEDIR + "data/" + svc;
   rg.createNode( hostName, svc, dbpath );
   rg.start();
   rg.removeNode( hostName, svc );

}

/*******************************************************************************
@Description : 检查list用户列表中用户名对应角色信息
@Modify 
*******************************************************************************/
function checkListUsers ( db, user, options )
{
   var cursor = db.list( SDB_LIST_USERS, { "User": user } );
   var count = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      assert.equal( obj.Options, options );
      count++;
   }

   assert.equal( count, 1 );
}

/*******************************************************************************
@Description : 检查list用户列表中用户名对应角色信息
@Modify 
*******************************************************************************/
function ddlAndDmlAndDqlOpr ( sdb, csName, clName )
{
   var dbcl = commCreateCL( sdb, csName, clName );
   dbcl.insert( { a: 1 } );
   var countNum = dbcl.find().count();
   assert.equal( countNum, 1 );
   sdb.dropCS( csName );
}

/*******************************************************************************
@Description : 选择一个编目节点建立连接
@Modify 
*******************************************************************************/
function getCatalogConn ( sdb, userName, passwd )
{
   var cataInfo = sdb.getRG( "SYSCatalogGroup" ).getDetail();
   var hostName = "";
   var svcName = "";
   while( cataInfo.next() )
   {
      var obj = cataInfo.current().toObj().Group;
      hostName = obj[0].HostName;
      svcName = obj[0].Service[0].Name;
   }
   cataInfo.close();
   var cataDB = new Sdb( hostName, svcName, userName, passwd );
   return cataDB;
}

/*******************************************************************************
@Description : 选择一个数据节点建立连接
@Modify 
*******************************************************************************/
function getDataConn ( sdb, userName, passwd )
{
   var groups = commGetGroups( sdb );
   var hostName = groups[0][1].HostName;
   var svcName = groups[0][1].svcname;
   var dataConn = new Sdb( hostName, svcName, userName, passwd );
   return dataConn;
}

/*******************************************************************************
@Description : 校验集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkStatistics ( actStatistics, expStatistics )
{
   for( var i = 0; i < expStatistics.length; i++ )
   {
      for( var j = 0; j < actStatistics.length; j++ )
      {
         if( expStatistics[i].NodeName === actStatistics[j].NodeName )
         {
            for( var key in expStatistics[j] )
            {
               assert.equal( expStatistics[i][key], actStatistics[j][key] );
            }
            break;
         }
      }
   }
}

/*******************************************************************************
@Description : 获取指定节点的集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getStatistics ( fullName, nodeNames )
{
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullName } );
   var tmpArray = [];
   var details = cursor.current().toObj().Details;
   for( var i = 0; i < details.length; i++ )
   {
      var group = cursor.current().toObj().Details[i].Group;
      for( var j = 0; j < nodeNames.length; j++ )
      {
         for( var k = 0; k < group.length; k++ )
         {
            if( group[k].NodeName === nodeNames[j] )
            {
               tmpArray.push( group[k] );
               break;
            }
         }
      }
   }
   return tmpArray;
}

/*******************************************************************************
@Description : 获取数据组名
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getDataGroupNames ()
{
   var groups = commGetGroups( db, false, "", false, false, false );
   var dataGroupNames = [];
   for( var i = 0; i < groups.length; i++ )
   {
      var tmpArray = groups[i];
      if( tmpArray[0].GroupName !== "SYSCatalogGroup" && tmpArray[0].GroupName !== "SYSCoord" )
      {
         dataGroupNames.push( tmpArray[0].GroupName );
      }
   }
   return dataGroupNames;
}

/*******************************************************************************
@Description : 校验实际结果与预期结果
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkResult ( actResult, expResult )
{
   assert.equal( actResult.length, expResult.length );
   assert.equal( actResult, expResult );
}


/************************************
*@Description: get a coord nod, a cata node, a data node
*@author:      zhaoxiaoni
*@createDate:  2019/10/21
**************************************/
function getNodeAddresses ()
{
   var cata = true;
   var data = true;
   var coord = true;
   var nodeAddresses = new Array();
   var cursor = db.listReplicaGroups();

   while( cursor.next() )
   {
      var groupObj = cursor.current().toObj();
      var groupArray = groupObj["Group"];
      for( var i = 0; i < groupArray.length; i++ )
      {
         var hostName = groupArray[i]["HostName"];
         var svcName = groupArray[i]["Service"][0]["Name"];
         var json = { "hostName": hostName, "svcName": svcName };
         var remote = new Remote( COORDHOSTNAME, 11790 );
         var cmd = remote.getCmd();
         var remoteHostName = cmd.run( "hostname" ).split( "\n" )[0];
         if( groupObj["Role"] === 1 && hostName != remoteHostName && coord == true )
         {
            nodeAddresses.push( json );
            coord = false;
            println( "coordNode is " + hostName + ":" + svcName );
         }
         else if( groupObj["Role"] === 2 && cata == true )
         {
            nodeAddresses.push( json );
            cata = false;
            println( "cataNode is " + hostName + ":" + svcName );
         }
         else if( groupObj["Role"] === 0 && data == true )
         {
            nodeAddresses.push( json );
            data = false;
            println( "dataNode is " + hostName + ":" + svcName );
         }
      }
   }
   return nodeAddresses;
}

function isContained ( actResult, expResult )
{
   var flag = true;
   for( var i = 0; i < expResult.length; i++ )
   {
      if( actResult.indexOf( expResult[i] ) == -1 )
      {
         flag = false;
         break;
      }
   }
   return flag;
}

function isNotContained ( actResult, expResult )
{
   var flag = true;
   for( var i = 0; i < expResult.length; i++ )
   {
      if( actResult.indexOf( expResult[i] ) !== -1 )
      {
         flag = false;
         break;
      }
   }
   return flag;
}

function checkParameters ( cursor, expResult )
{
   while( cursor.next() )
   {
      var object = cursor.current().toObj().Details[0];
      for( var i in expResult )
      {
         assert.equal( object[i], expResult[i] );
      }
   }
}

function getCursorResult ( cursor )
{
   var cursorResult = [];
   while( cursor.next() )
   {
      cursorResult.push( cursor.current().toObj()["Name"] );
   }
   cursor.close();
   return cursorResult;
}

/*******************************************************************************
@Description : 插入测试数据
@Modify list : 2020-05-31 wuyan init
*******************************************************************************/
function insertRecs ( dbcl, recsNum )
{
   if( typeof ( recsNum ) == "undefined" ) { recsNum = 100; }
   var doc = [];
   for( var i = 0; i < recsNum; i++ )
   {
      doc.push( { a: i, no: i, b: i, c: "test" + i } );
   }
   dbcl.insert( doc );
}

/*******************************************************************************
@Description : 直连主节点获取集合快照
@Modify list : 2020-05-31 wuyan init
*******************************************************************************/
function getCLSnapshotFromMasterNode ( groupName, clName )
{
   try
   {
      var masterNode = db.getRG( groupName ).getMaster();
      var masterdb = new Sdb( masterNode.getHostName(), masterNode.getServiceName() );
      var clSnapshotInfo = masterdb.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   }
   finally
   {
      if( masterdb !== undefined )
      {
         masterdb.close();
      }
   }
   return clSnapshotInfo;
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getCoordRG().getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}

/************************************
*@Description: try to drop user by name
*@author:      tangtao
*@createDate:  2022/09/26
**************************************/
function cleanUsers ( user )
{
   try
   {
      db.dropUsr( user, user ) ;
   }
   catch( e )
   {
      if( e.message != SDB_AUTH_USER_NOT_EXIST )
      {
         throw new Error( e ) ;
      }
   }
}