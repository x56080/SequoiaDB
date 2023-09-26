/******************************************************************************
 * @Description   : seqDB-33479:节点处于运维模式，查看快照
 * @Author        : liuli
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var group = db.getRG( groupName );

   // group1所有备节点启动运维模式
   var slaveNodeNames = getGroupSlaveNodeName( db, groupName );
   for( var i in slaveNodeNames )
   {
      var options = { NodeName: slaveNodeNames[i], MinKeepTime: 10, MaxKeepTime: 20 };
      group.startMaintenanceMode( options );
   }

   // 查看数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, GroupName: groupName }, { NodeName: 1 } );
   var snapshotNodes = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      snapshotNodes.push( obj["NodeName"] );
   }
   cursor.close();

   var masterNodeName = getGroupMasterNodeName( db, groupName )[0];
   var expNodes = slaveNodeNames;
   expNodes.push( masterNodeName );

   assert.equal( snapshotNodes.sort(), expNodes.sort() );

   // 查看节点健康快照
   var cursor = db.snapshot( SDB_SNAP_HEALTH, { RawData: true, GroupName: groupName }, { NodeName: 1 } );
   var snapshotNodes = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      snapshotNodes.push( obj["NodeName"] );
   }
   cursor.close();

   assert.equal( snapshotNodes.sort(), expNodes.sort() );

   // 直连数据节点查看数据库
   var node = group.getNode( slaveNodeNames[0] ).connect();
   var cursor = node.snapshot( SDB_SNAP_DATABASE );
   if( !cursor.next() )
   {
      throw new Error( "snapshot failed" );
   }
   cursor.close();

   // 直连数据节点查看健康快照
   var cursor = node.snapshot( SDB_SNAP_HEALTH );
   if( !cursor.next() )
   {
      throw new Error( "snapshot failed" );
   }
   cursor.close();

   node.close();

   group.stopMaintenanceMode();
}