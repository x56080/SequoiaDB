/*******************************************************************************
@Description : 校验集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkStatistics ( actStatistics, expStatistics )
{
   try
   {
      println( "Start to check statistics" );
      for( var i = 0; i < expStatistics.length; i++ )
      {
         for( var j = 0; j < actStatistics.length; j++ )
         {
            if( expStatistics[i].NodeName === actStatistics[j].NodeName )
            {
               for( var key in expStatistics[j] )
               {
                  if( expStatistics[i][key] !== actStatistics[j][key] )
                  {
                     throw "expect: " + expStatistics[j][key] + "\nact: " + actStatistics[j][key];
                  }
               }
               break;
            }
         }
      }
      println( "End to check statistics" );
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 获取指定节点的集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getStatistics ( fullName, nodeNames )
{
   try
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
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 获取数据组名
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getDataGroupNames ()
{
   var groups = commGetGroups( db, false, "", false, false, false );
   try
   {
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
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 校验实际结果与预期结果
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkResult ( actResult, expResult )
{
   try
   {
      if( actResult.length !== expResult.length )
      {
         throw "expectResult.length is " + expResult.length + ", but actResult.length is " + actResult.length;
      }
      if( JSON.stringify( actResult ) !== JSON.stringify( expResult ) )
      {
         throw "expectResult is " + JSON.stringify( expResult ) + ", but actResult is " + JSON.stringify( actResult );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}


/************************************
*@Description: get a coord nod, a cata node, a data node
*@author:      zhaoxiaoni
*@createDate:  2019/10/21
**************************************/
function getNodeAddresses ()
{
   try 
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
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 停止节点
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function stopNodes ( nodeAddresses )
{
   var installDir = commGetInstallPath();
   try
   {
      command = installDir + "/bin/sdbstop -p ";

      for( var i = 0; i < nodeAddresses.length; i++ )
      {
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         var remote = new Remote( hostName, 11790 );
         var cmd = remote.getCmd();
         cmd.run( command + svcName );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 开启节点
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function startNodes ( nodeAddresses )
{
   var installDir = commGetInstallPath();
   try
   {
      command = installDir + "/bin/sdbstart -p ";

      for( var i = 0; i < nodeAddresses.length; i++ )
      {
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         var remote = new Remote( hostName, 11790 );
         var cmd = remote.getCmd();
         cmd.run( command + svcName );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function isContained( actResult, expResult )
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

function isNotContained( actResult, expResult )
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

function checkParameters( cursor, expResult )
{
   while( cursor.next() )
   {
      var object = cursor.current().toObj().Details[0];
      for( var i in expResult )
      {
         if( object[i] !== expResult[i] )
         {
            throw new Error( "\nact " + i + ": " + JSON.stringify( object[i] ) + "\nexp " + i + ": " + JSON.stringify( expResult[i] ) );
         }
      }
   }
}

function getCursorResult( cursor )
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
function insertRecs( dbcl, recsNum )
{
   if( typeof ( recsNum ) == "undefined" ) { recsNum = 100; }
   var doc = [];
   for( var i = 0; i < recsNum; i++ )
   {
      doc.push( { a: i, no: i, b:i, c: "test"+ i } );
   }
   dbcl.insert( doc );
}

/*******************************************************************************
@Description : 直连主节点获取集合快照
@Modify list : 2020-05-31 wuyan init
*******************************************************************************/
function getCLSnapshotFromMasterNode(groupName, clName)
{
   try
   {
      var masterNode = db.getRG( groupName ).getMaster();
      var masterdb = new Sdb( masterNode.getHostName(),masterNode.getServiceName());      
      var clSnapshotInfo = masterdb.snapshot(SDB_SNAP_COLLECTIONS,{ Name:COMMCSNAME + "." + clName}).next().toObj();
   }
   finally
   {
      if( masterdb !== undefined ){
         masterdb.close();
      }      
   }   
   return clSnapshotInfo;
}
