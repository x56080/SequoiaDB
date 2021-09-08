/******************************************************************************
 * @Description   : seqDB-21881:内置SQL语句查询$SNAPSHOT_CONTEXT
 *                  seqDB-21882:内置SQL语句查询$SNAPSHOT_CONTEXT_CUR
 * @Author        : liyuanyue
 * @CreateTime    : 2020.03.20
 * @LastEditTime  : 2021.09.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var obj = [];
   var cur = db.exec( "select * from $SNAPSHOT_CONTEXT" );
   while( cur.next() )
   {
      obj.push( cur.current().toObj() );
   }
   cur.close();

   for( var i = 0; i < obj.length; i++ )
   {
      var tmpObj = obj[i];
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_CONTEXTS, { NodeName: tmpObj["NodeName"], SessionID: tmpObj["SessionID"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var expObj = [];
         var actObj = [];
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expContexts = tmpObj["Contexts"];
         var actContexts = snapshotTmpObj["Contexts"];
         expContexts.sort( sortBy( "ContextID" ) );
         actContexts.sort( sortBy( "ContextID" ) );
         // 排序后选取Contexts中部分字段进行校验
         assert.equal( expContexts.length, actContexts.length, "exp : " + JSON.stringify( expContexts, 0, 1 ) + "\nact : " + JSON.stringify( actContexts ) );
         for( var j = 0; j < expContexts.length; j++ )
         {
            expObj.push( {
               Type: tmpObj["Contexts"][j]["Type"], Description: tmpObj["Contexts"][j]["Description"],
               DataRead: tmpObj["Contexts"][j]["DataRead"], IndexRead: tmpObj["Contexts"][j]["IndexRead"], QueryTimeSpent: tmpObj["Contexts"][j]["QueryTimeSpent"]
            } );
            actObj.push( {
               Type: snapshotTmpObj["Contexts"][j]["Type"],
               Description: snapshotTmpObj["Contexts"][j]["Description"], DataRead: snapshotTmpObj["Contexts"][j]["DataRead"],
               IndexRead: snapshotTmpObj["Contexts"][j]["IndexRead"], QueryTimeSpent: snapshotTmpObj["Contexts"][j]["QueryTimeSpent"]
            } );
         }
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CONTEXT result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "expect snapshotCount 1, actual snapshotCount is " + snapshotCount + "\ntmpObj:" + JSON.stringify( tmpObj ) + "\nsnapshotTmpObj:" + JSON.stringify( snapshotTmpObj ) );
      }
   }

   // 使用内置SQL语句查询快照信息
   var obj = [];
   var cur = db.exec( "select * from $SNAPSHOT_CONTEXT_CUR" );
   while( cur.next() )
   {
      obj.push( cur.current().toObj() );
   }
   cur.close();

   for( var i = 0; i < obj.length; i++ )
   {
      var tmpObj = obj[i];
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_CONTEXTS_CURRENT, { NodeName: tmpObj["NodeName"], SessionID: tmpObj["SessionID"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var expObj = [];
         var actObj = [];
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expContexts = tmpObj["Contexts"];
         var actContexts = snapshotTmpObj["Contexts"];
         expContexts.sort( sortBy( "ContextID" ) );
         actContexts.sort( sortBy( "ContextID" ) );
         // 排序后选取Contexts中部分字段进行校验
         assert.equal( expContexts.length, actContexts.length, "exp : " + JSON.stringify( expContexts, 0, 1 ) + "\nact : " + JSON.stringify( actContexts ) );
         for( var j = 0; j < expContexts.length; j++ )
         {
            expObj.push( {
               Type: tmpObj["Contexts"][j]["Type"], Description: tmpObj["Contexts"][j]["Description"],
               DataRead: tmpObj["Contexts"][j]["DataRead"], IndexRead: tmpObj["Contexts"][j]["IndexRead"], QueryTimeSpent: tmpObj["Contexts"][j]["QueryTimeSpent"]
            } );
            actObj.push( {
               Type: snapshotTmpObj["Contexts"][j]["Type"],
               Description: snapshotTmpObj["Contexts"][j]["Description"], DataRead: snapshotTmpObj["Contexts"][j]["DataRead"],
               IndexRead: snapshotTmpObj["Contexts"][j]["IndexRead"], QueryTimeSpent: snapshotTmpObj["Contexts"][j]["QueryTimeSpent"]
            } );
         }
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CONTEXT_CUR result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "expect snapshotCount 1, actual snapshotCount is " + snapshotCount + "\ntmpObj:" + JSON.stringify( tmpObj ) + "\nsnapshotTmpObj:" + JSON.stringify( snapshotTmpObj ) );
      }
   }
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}