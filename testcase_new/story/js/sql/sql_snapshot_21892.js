/******************************************************************************
@Description seqDB-21892:内置SQL语句查询$SNAPSHOT_ACCESSPLANS
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_21892";

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } )
   }
   dbcl.insert( docs );

   dbcl.find().toArray();

   for( var i = 0; i < 10; i++ )
   {
      dbcl.find( { a: i } ).toArray();
   }

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_ACCESSPLANS where Collection = '" + COMMCSNAME + "." + testConf.clName + "'" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_ACCESSPLANS, { NodeName: tmpObj["NodeName"], AccessPlanID: tmpObj["AccessPlanID"] } );
      var snapshotCount = 0;
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            GroupName: tmpObj["GroupName"], Collection: tmpObj["Collection"],
            CollectionSpace: tmpObj["CollectionSpace"], Score: tmpObj["Score"]
         };
         var actObj = {
            GroupName: snapshotTmpObj["GroupName"], Collection: snapshotTmpObj["Collection"],
            CollectionSpace: snapshotTmpObj["CollectionSpace"], Score: snapshotTmpObj["Score"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_ACCESSPLANS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
   }
}
