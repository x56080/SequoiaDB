import( "../lib/main.js" );

/******************************************************************************
 * @description: 获取复制组的详细信息进行校验
 * @param {string} groupName  // 复制组名
 * @param {string} nodeName  // 节点名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function checkLocationDeatil ( db, groupName, nodeName, location )
{
   var rg = db.getRG( groupName );
   var obj = rg.getDetailObj().toObj();
   var locations = obj.Locations;
   var groupInfo = obj.Group;

   var actLocations = [];
   for( var i in locations )
   {
      var actLocation = locations[i]["Location"];
      actLocations.push( actLocation );
   }

   if( location != undefined )
   {
      // 当传入的location不在actLacation中时，报错
      if( actLocations.indexOf( location ) == -1 )
      {
         throw new Error( "expect location : " + location + " ,actual locations : " + actLocations );
      }
   }

   var checkNode = false;
   if( nodeName != undefined )
   {
      for( var i in groupInfo )
      {
         var hostName = groupInfo[i]["HostName"];
         var services = groupInfo[i]["Service"];
         for( var j in services )
         {
            if( services[j]["Type"] == 0 )
            {
               var serviceName = services[j]["Name"];
            }
         }
         var actNodeName = hostName + ":" + serviceName;
         if( actNodeName == nodeName )
         {
            var nodeLocation = groupInfo[i]["Location"];
            assert.equal( nodeLocation, location );
            var checkNode = true;
         }
      }
      // 当遍历完所有group没有得到需要匹配的节点，报错
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}

/******************************************************************************
 * @description: 获取复制组节点的详细信息进行校验
 * @param {string} node  // 节点名
 * @param {string} expLocation  // 需要校验的location
 ******************************************************************************/
function checkNodeLocation ( node, expLocation )
{
   var nodeObj = node.getDetailObj().toObj();
   actLocation = nodeObj.Location;
   assert.equal( actLocation, expLocation );
}

/******************************************************************************
 * @description: 获取复制组信息进行校验
 * @param {SdbCursor} cursor  
 * @param {string} groupName  // 复制组名
 * @param {string} nodeName  // 节点名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function checkLocationToGroup ( cursor, groupName, nodeName, location )
{
   while( cursor.next() )
   {
      // 游标遍历复制组
      var actGroupName = cursor.current().toObj().GroupName;
      // 当游标中的复制组与校验复制组相等时，获取出locations内容与Group内容
      if( actGroupName == groupName )
      {
         var locations = cursor.current().toObj().Locations;
         var groupInfo = cursor.current().toObj().Group;
      }
   }
   cursor.close();

   var actLocations = [];
   for( var i in locations )
   {
      // 获取Locations字段中的location存入actLocations中
      var actLocation = locations[i]["Location"];
      actLocations.push( actLocation );
   }

   if( location != undefined )
   {
      // 当传入的location不在actLacation中时，报错
      if( actLocations.indexOf( location ) == -1 )
      {
         throw new Error( "expect location : " + location + " ,actual locations : " + actLocations );
      }
   }

   var checkNode = false;
   if( nodeName != undefined )
   {
      for( var i in groupInfo )
      {
         var hostName = groupInfo[i]["HostName"];
         var services = groupInfo[i]["Service"];
         for( var j in services )
         {
            if( services[j]["Type"] == 0 )
            {
               var serviceName = services[j]["Name"];
            }
         }
         var actNodeName = hostName + ":" + serviceName;
         // 判断传入的节点与需要校验的节点是否相等
         if( actNodeName == nodeName )
         {
            var nodeLocation = groupInfo[i]["Location"];
            assert.equal( nodeLocation, location );
            var checkNode = true;
         }
      }
      // 当遍历完所有group没有得到需要匹配的节点，报错
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}

/******************************************************************************
 * @description: 获取复制组中locations中的locationID
 * @param {string} groupName  // 复制组名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function getLocationID ( db, groupName, location )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName }, { Locations: "" } );
   var locationID = -1;
   while( cursor.next() )
   {
      var locations = cursor.current().toObj().Locations;
      for( var i in locations )
      {
         var actLocation = locations[i]["Location"];
         if( actLocation == location )
         {
            var locationID = locations[i]["LocationID"];
            break;
         }
      }
      if( locationID != -1 )
      {
         break;
      }
   }
   cursor.close();
   assert.notEqual( locationID, -1, "location does not exist" );
   return locationID;
}

/******************************************************************************
 * @description: 获取复制组中的version字段
 * @param {string} groupName  // 复制组名
 ******************************************************************************/
function getGroupVersion ( db, groupName )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName }, { Version: "" } );
   while( cursor.next() )
   {
      var version = cursor.current().toObj().Version;
   }
   cursor.close();
   return version;
}

function compareSize ( minSize, maxSize )
{
   if( minSize >= maxSize )
   {
      throw new Error( "minSize:" + minSize + ", maxSize:" + minSize );
   }
}

/******************************************************************************
 * @description: 移除节点
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 ******************************************************************************/
function removeNode ( rg, hostName, port )
{
   try
   {
      rg.removeNode( hostName, port );
   }
   catch( e )
   {
      if( e != SDB_CLS_NODE_NOT_EXIST )
      {
         throw e;
      }
   }
}

/******************************************************************************
 * @description: 将节点加入当前复制组
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 * @param {json} option  // 设置是否保留新加节点原有的数据
 ******************************************************************************/
function attachNode ( rg, hostName, port, option )
{
   try
   {
      rg.attachNode( hostName, port, option );
   }
   catch( e )
   {
      if( e != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
}