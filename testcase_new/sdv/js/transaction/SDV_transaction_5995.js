/* *****************************************************************************
@discretion: ִ������ͷ�����������ύ����
@author��2015-11-18 wuyan  Init
***************************************************************************** */

main();
function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_transaction5995";
      if( !commIsTransEnabled( db ) )
      {
         println( "transaction is disabled" );
         return;
      }

      beginTrans();
      var cl = commCreateCL( db, COMMCSNAME, clName, 0, false, true, true );
      commCreateIndex( cl, "testIndex", { no: 1 }, true, true );
      var dataNum = 1000;
      var insert = new insertData( cl, dataNum );
      var update = new updateData( cl );
      //remove data and left some datas,then commit transaction
      var removeNum = 100;
      var remove = new removeData( cl, removeNum );
      execTransaction( insert, update );
      checkResult( cl, true, update );
      execTransaction( remove, commitTrans );
      checkResult( cl, true, remove );

      //@ clean end
      commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
}



