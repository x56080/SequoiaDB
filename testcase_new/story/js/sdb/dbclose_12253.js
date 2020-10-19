/******************************************************************************
*@Description : test db operation after close
*               TestLink : seqDB-12253 ����close��ִ�в���
*@auhor       : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   db.close();

   assert.tryThrow( [-64, -6], function()
   {
      db.traceResume();
   } );
}

