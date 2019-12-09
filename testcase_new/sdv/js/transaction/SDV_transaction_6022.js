/************************************************************************
*@Description:	seqDB-6022��ִ������ͷ�����������ع�����_SD.transaction.033
               ����ִ�п������񡢴���cscl����ɾ�ġ��ع�
*@Author:  		TingYU  2015/11/23
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCLNAME + "_yt6022";
      var clName = COMMCLNAME + "_yt6022";

      if( !commIsTransEnabled( db ) )
      {
         println( "transaction is disabled" );
         return;
      }

      execTransaction( beginTrans );

      var cl = createCSCL( csName, clName, { ReplSize: 0 } );

      var dataNum = 5000;
      var insert = new insertData( cl, dataNum );
      var update = new updateData( cl );
      var remove = new removeData( cl );
      execTransaction( insert );
      checkResult( cl, true, insert );
      execTransaction( update );
      checkResult( cl, true, update );
      execTransaction( remove );
      checkResult( cl, true, remove );

      execTransaction( rollbackTrans );
      checkResult( cl, false, insert );

      clean( csName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
   }
}

function createCSCL ( csName, clName, option )
{
   println( "--create cs and cl" );

   commDropCS( db, csName, true, "drop cs in ready" );
   commCreateCS( db, csName, false, "create cs  in begin" );
   var cl = commCreateCLByOption( db, csName, clName, option, false, false, "create cl in begin" );

   return cl;
}

function clean ( csName )
{
   println( "--clean" );

   commDropCS( db, csName, false, "drop cs in clean" );
} 