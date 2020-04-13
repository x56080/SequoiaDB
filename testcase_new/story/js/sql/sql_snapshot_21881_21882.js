/******************************************************************************
@Description seqDB-21881:内置SQL语句查询$SNAPSHOT_CONTEXT
             seqDB-21882:内置SQL语句查询$SNAPSHOT_CONTEXT_CUR
@author liyuanyue
@date 2020-3-20
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_CONTEXT" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_CONTEXTS, { NodeName: tmpObj["NodeName"], SessionID: tmpObj["SessionID"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            Type: tmpObj["Contexts"][0]["Type"], Description: tmpObj["Contexts"][0]["Description"],
            DataRead: tmpObj["Contexts"][0]["DataRead"], IndexRead: tmpObj["Contexts"][0]["IndexRead"], QueryTimeSpent: tmpObj["Contexts"][0]["QueryTimeSpent"]
         };
         var actObj = {
            Type: snapshotTmpObj["Contexts"][0]["Type"],
            Description: snapshotTmpObj["Contexts"][0]["Description"], DataRead: snapshotTmpObj["Contexts"][0]["DataRead"],
            IndexRead: snapshotTmpObj["Contexts"][0]["IndexRead"], QueryTimeSpent: snapshotTmpObj["Contexts"][0]["QueryTimeSpent"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CONTEXT result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "Node does not exist in snapshot result" );
      }
   }

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_CONTEXT_CUR" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_CONTEXTS_CURRENT, { NodeName: tmpObj["NodeName"], SessionID: tmpObj["SessionID"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            Type: tmpObj["Contexts"][0]["Type"], Description: tmpObj["Contexts"][0]["Description"],
            DataRead: tmpObj["Contexts"][0]["DataRead"], IndexRead: tmpObj["Contexts"][0]["IndexRead"], QueryTimeSpent: tmpObj["Contexts"][0]["QueryTimeSpent"]
         };
         var actObj = {
            Type: snapshotTmpObj["Contexts"][0]["Type"],
            Description: snapshotTmpObj["Contexts"][0]["Description"], DataRead: snapshotTmpObj["Contexts"][0]["DataRead"],
            IndexRead: snapshotTmpObj["Contexts"][0]["IndexRead"], QueryTimeSpent: snapshotTmpObj["Contexts"][0]["QueryTimeSpent"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CONTEXT_CUR result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "Node does not exist in snapshot result" );
      }
   }
}