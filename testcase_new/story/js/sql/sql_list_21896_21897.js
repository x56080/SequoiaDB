/******************************************************************************
@Description seqDB-21896:内置SQL语句查询$LIST_CONTEXT
             seqDB-21897:内置SQL语句查询$LIST_CONTEXT_CUR
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_CONTEXT" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.list( SDB_LIST_CONTEXTS, { NodeName: tmpObj["NodeName"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = { SessionID: tmpObj["SessionID"], TotalCount: tmpObj["TotalCount"] };
         var actObj = { SessionID: snapshotTmpObj["SessionID"], TotalCount: snapshotTmpObj["TotalCount"] };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_CONTEXT result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "Node does not exist in snapshot result" );
      }
   }

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_CONTEXT_CUR" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.list( SDB_LIST_CONTEXTS_CURRENT, { NodeName: tmpObj["NodeName"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = { SessionID: tmpObj["SessionID"], TotalCount: tmpObj["TotalCount"] };
         var actObj = { SessionID: snapshotTmpObj["SessionID"], TotalCount: snapshotTmpObj["TotalCount"] };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_CONTEXT_CUR result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "Node does not exist in snapshot result" );
      }
   }
}
