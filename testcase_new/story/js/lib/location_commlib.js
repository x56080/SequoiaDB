import( "../lib/main.js" );

// 获取复制组的详细信息进行校验
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
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}
// 获取复制组节点的详细信息进行校验
function checkNodeLocation ( node, expLocation )
{
   var nodeObj = node.getDetailObj().toObj();
   actLocation = nodeObj.Location;
   assert.equal( actLocation, expLocation );
}
// 获取复制组信息进行校验
function checkLocationToGroup ( cursor, groupName, nodeName, location )
{
   while( cursor.next() )
   {
      var actGroupName = cursor.current().toObj().GroupName;
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
      var actLocation = locations[i]["Location"];
      actLocations.push( actLocation );
   }

   if( location != undefined )
   {
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
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}

//获取复制组中locations中的locationID
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

//获取复制组中的version字段
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

//移除节点
function removeND ( rg, hostName, port )
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