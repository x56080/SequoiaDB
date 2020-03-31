/******************************************************************************
@Description seqDB-21904:内置SQL语句查询$LIST_TRANS
             seqDB-21905:内置SQL语句查询$LIST_TRANS_CUR
@author liyuanyue
@date 2020-3-24
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_21904_21905";

main( test );

function test ()
{
   db.transBegin();
   testPara.testCL.insert( { a: 1 } );

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_TRANS" );
   var transCount = 0;
   while( cur.next() )
   {
      transCount++;
   }

   if( transCount != 1 )
   {
      throw new Error( "$LIST_TRAN result error\nexpected result is 1, but actually result is " + transCount );
   }

   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_TRANS_CUR" );
   var transCount = 0;
   while( cur.next() )
   {
      transCount++;
   }

   if( transCount != 1 )
   {
      throw new Error( "$LIST_TRANS_CUR result error\nresult is 1, but actually result is " + transCount );
   }

   db.transRollback();
}