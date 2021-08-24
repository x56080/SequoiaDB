/******************************************************************************
 * @Description   : seqDB-24324:指定showError查询数据库快照，不存在异常节点
 * @Author        : liuli
 * @CreateTime    : 2021.08.24
 * @LastEditTime  : 2021.08.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "show" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   var errNodes = cursor.current().toObj()["ErrNodes"];
   assert.equal( errNodes, [] );

   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "ignore" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   var errNodes = cursor.current().toObj()["ErrNodes"];
   assert.equal( errNodes, undefined );

   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "only" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   while( cursor.next() )
   {
      throw new Error( "should error but success\n" + JSON.stringify( cursor.current().toObj(), 0, 1 ) );
   }
}