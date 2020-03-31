/******************************************************************************
@Description seqDB-21890:内置SQL语句查询$SNAPSHOT_TRANS
             seqDB-21891:内置SQL语句查询$SNAPSHOT_TRANS_CUR
@author liyuanyue
@date 2020-3-19
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_21890_21891";

main( test );

function test ()
{
   db.transBegin();
   testPara.testCL.insert( { a: 1 } );

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_TRANS" );
   var transCount = 0;
   while( cur.next() )
   {
      transCount++;
   }

   if( transCount != 1 )
   {
      throw new Error( "expected result is 1, but actually result is " + transCount );
   }

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_TRANS_CUR" );
   var transCount = 0;
   while( cur.next() )
   {
      transCount++;
   }

   if( transCount != 1 )
   {
      throw new Error( "result is 1, but actually result is " + transCount );
   }

   db.transRollback();
}
