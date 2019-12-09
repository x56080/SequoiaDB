/* *****************************************************************************
@discretion: �ظ���������,ִ������������ύ
@author��2015-11-17 wuyan  Init
***************************************************************************** */
main();
function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_transaction5991";
      if( !commIsTransEnabled( db ) )
      {
         println( "transaction is disabled" );
         return;
      }

      var cl = commCreateCL( db, COMMCSNAME, clName, 0, false, true, true );
      var dataNum = 1000;
      var insert = new insertData( cl, dataNum );
      execTransaction( beginTrans, insert, beginTrans, commitTrans );
      checkResult( cl, true, insert );

      //@ clean end
      commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
}


