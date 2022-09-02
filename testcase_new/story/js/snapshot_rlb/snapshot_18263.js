/******************************************************************************
 * @Description   : seqDB-18263:指定快照查询参数Mode为Run查询配置快照信息
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.24
 * @LastEditTime  : 2022.09.02
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
main( test );

function test ( testPara )
{
   var groups = testPara.groups;
   var groupName = groups[0][0].GroupName;
   var hostName = groups[0][1].HostName;
   var svcname = groups[0][1].svcname;
   var nodeName = hostName + ":" + svcname;

   changeConf( nodeName );
   db.getRG( groupName ).getSlave().stop();
   db.getRG( groupName ).getSlave().start();

   var expResult = [{ "transactionon": "FALSE" }];
   var option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
   checkResult( option, expResult );

   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      db.deleteConf( { transactionon: 1 }, { 'NodeName': nodeName } );
   } );

   expResult = [{ "transactionon": "FALSE" }];
   option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
   checkResult( option, expResult );

   expResult = [{}];
   option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "local", "expand": false } );
   checkResult( option, expResult );
}

function changeConf ( nodeName )
{
   try
   {
      db.updateConf( { transactionon: false }, { NodeName: nodeName } );
   }
   catch( e )
   {
      if( e.message != SDB_RTN_CONF_NOT_TAKE_EFFECT )
      {
         throw new Error( e );
      }
   }
}

function checkResult ( option, expResult )
{
   var actResult = [];
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, option );
   while( cursor.next() )
   {
      actResult.push( { "transactionon": cursor.current().toObj().transactionon } );
   }
   assert.equal( actResult.length, expResult.length );
   assert.equal( actResult, expResult, "实际结果与预期结果一致" );
}