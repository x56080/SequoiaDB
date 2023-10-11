/******************************************************************************
 * @Description   : seqDB-31764:2副本异常，指定Enforced为true强制切主
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.05.24
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31764";
testConf.clOpt = { ReplSize: -1 };

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;
   var clName2 = COMMCLNAME + "_31764_2";
   var clName3 = COMMCLNAME + "_31764_3";

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点与备节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();

   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   try
   {
      slaveNode1.stop();
      insertBulkData( dbcl, 100000 );

      slaveNode1.start();
      //节点启动后，需要收到所有节点的心跳信息才会进入选举流程
      //这里如果节点刚启动，就把另外两个节点强杀了。启动的节点要等待600s 才会进入选举的流程
      sleep( 10000 );
      killNode( db, masterNode );
      killNode( db, slaveNode2 );

      // 剩余一个节点启动Critical模式
      var minKeepTime = 1;
      var maxKeepTime = 10;
      var options = { NodeName: slaveNodes[0], MinKeepTime: minKeepTime,
                      MaxKeepTime: maxKeepTime, Enforced: true };
      rg.startCriticalMode( options );
      var beginTime = new Date();

      // 插入数据并校验
      var options = { ReplSize: 0, Group : srcGroup };
      var dbcl2 = args.testCS.createCL( clName2, options );
      var docs2 = insertBulkData( dbcl2, 1000 );
      var cursor = dbcl2.find().sort( { a: 1 } );
      commCompareResults( cursor, docs2 );

      // 启动节点,停止Critical模式
      masterNode.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );
      rg.stopCriticalMode();

      // 插入数据并校验
      var options = { ReplSize: 0, Group : srcGroup };
      var dbcl3 = args.testCS.createCL( clName3, options );
      var docs3 = insertBulkData( dbcl3, 1000 );
      var cursor = dbcl3.find().sort( { a: 1 } );
      commCompareResults( cursor, docs3 );

   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
      commDropCL( db, testConf.csName, clName2, true, true );
      commDropCL( db, testConf.csName, clName3, true, true );
   }
}