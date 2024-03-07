/******************************************************************************
 * @Description   : seqDB-34017:多副本集群，停止一个副本，createCL指定ReplSize:-1
 * @Author        : huangxiaoni
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2024.03.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var dataGroupNames = commGetDataGroupNames( db );
   var groupName = dataGroupNames[0];
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_" + "cl_34017";

   var cs = db.getCS( csName );

   var slaveNode = db.getRG( groupName ).getSlave();
   try
   {
      // 停止一个备节点
      slaveNode.stop();

      // 建表
      commDropCL( db, csName, clName, true );
      var cl = cs.createCL( clName, { ReplSize: -1, Group: groupName } );

      // 读写数据
      cl.insert( { a: 1 } );
      assert.equal( cl.count(), 1 );
   }
   finally
   {
      slaveNode.start();
   }
   commDropCL( db, csName, clName, false );
}