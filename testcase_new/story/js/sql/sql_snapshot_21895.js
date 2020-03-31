/******************************************************************************
@Description seqDB-21895:内置SQL语句查询$SNAPSHOT_SVCTASKS
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用 snapshot 语句查询快照信息
   var cur = db.snapshot( SDB_SNAP_SVCTASKS );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // 将内置 sql 执行结果累计验证
      var totalIndexRead = db.exec( "select SUM(TotalIndexRead) as totalIndexRead from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalIndexRead"];
      var totalDataWrite = db.exec( "select SUM(TotalDataWrite) as totalDataWrite from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalDataWrite"];
      var totalIndexWrite = db.exec( "select SUM(TotalIndexWrite) as totalIndexWrite from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalIndexWrite"];
      var totalUpdate = db.exec( "select SUM(TotalUpdate) as totalUpdate from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalUpdate"];
      var totalDelete = db.exec( "select SUM(TotalDelete) as totalDelete from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalDelete"];
      var totalInsert = db.exec( "select SUM(TotalInsert) as totalInsert from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalInsert"];
      var totalWrite = db.exec( "select SUM(TotalWrite) as totalWrite from $SNAPSHOT_SVCTASKS " ).current().toObj()["totalWrite"];
      var actObj = {
         TotalIndexRead: tmpObj["TotalIndexRead"], TotalDataWrite: tmpObj["TotalDataWrite"], TotalIndexWrite: tmpObj["TotalIndexWrite"],
         TotalUpdate: tmpObj["TotalUpdate"], TotalDelete: tmpObj["TotalDelete"], TotalInsert: tmpObj["TotalInsert"], TotalWrite: tmpObj["TotalWrite"]
      };
      var expObj = {
         TotalIndexRead: totalIndexRead, TotalDataWrite: totalDataWrite, TotalIndexWrite: totalIndexWrite, TotalUpdate: totalUpdate,
         TotalDelete: totalDelete, TotalInsert: totalInsert, TotalWrite: totalWrite
      };
      if( !( commCompareObject( expObj, actObj ) ) )
      {
         throw new Error( "$SNAPSHOT_SVCTASKS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
      }
   }
}
