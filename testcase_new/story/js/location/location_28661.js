/******************************************************************************
 * @Description   : seqDB-28661:data节点设置Location后，分离节点
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.16
 * @LastEditTime  : 2022.11.17
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_28659";
   var groupName = "group1";
   var groupsArray = commGetGroups( db );
   var hostName = groupsArray[0][1].HostName;
   var port1 = parseInt( RSRVPORTBEGIN ) + 10;
   var port2 = parseInt( RSRVPORTBEGIN ) + 20;
   var dbpath1 = "data/" + port1;
   var dbpath2 = "data/" + port2;

   try
   {
      var dataRG = db.getRG( groupName );
      dataRG.createNode( hostName, port1, dbpath1, { diaglevel: 5 } );
      dataRG.createNode( hostName, port2, dbpath2, { diaglevel: 5 } );
      dataRG.start();

      var data1 = dataRG.getNode( hostName, port1 );
      var data2 = dataRG.getNode( hostName, port2 );

      var nodeName1 = data1.getHostName() + ":" + data1.getServiceName();
      var nodeName2 = data2.getHostName() + ":" + data2.getServiceName();

      // 设置备节点1的location
      data1.setAttributes( { Location: location } );
      var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
      checkLocationToGroup( cursor, groupName, nodeName1, location );
      var groupVersion1 = getGroupVersion( db, groupName );
      var locationID1 = getLocationID( db, groupName, location );

      // 分离备节点1
      dataRG.detachNode( hostName, port1, { KeepData: false } );
      var groupVersion2 = getGroupVersion( db, groupName );
      compareSize( groupVersion1, groupVersion2 );

      // 设置备节点2为同名的location
      data2.setAttributes( { Location: location } );
      var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
      checkLocationToGroup( cursor, groupName, nodeName2, location );
      var groupVersion3 = getGroupVersion( db, groupName );
      var locationID2 = getLocationID( db, groupName, location );
      compareSize( groupVersion2, groupVersion3 );
      compareSize( locationID1, locationID2 );

      //分离备节点2
      dataRG.detachNode( hostName, port2, { KeepData: false } );
      var groupVersion3 = getGroupVersion( db, groupName );
      compareSize( groupVersion2, groupVersion3 );

      dataRG.attachNode( hostName, port1, { KeepData: false } );
      dataRG.attachNode( hostName, port2, { KeepData: false } );
   }
   finally
   {
      try
      {
         dataRG.removeNode( hostName, port1 );
      }
      catch( e )
      {
         if( e.message != SDB_CLS_NODE_NOT_EXIST )
         {
            throw e;
         }
      }
      try
      {
         dataRG.removeNode( hostName, port2 );
      }
      catch( e )
      {
         if( e.message != SDB_CLS_NODE_NOT_EXIST )
         {
            throw e;
         }
      }
   }
}