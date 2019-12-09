/* *****************************************************************************
@discretion: rename cs
             seqDB-16111 �޸�cs��ָ��cs��Ϊԭ�������Ѵ��ڵ�cs
@author��2018-10-13 chensiqin  Init
***************************************************************************** */
main( db );

function main ( db )
{
   /*
    1������cs 2���޸�cs��Ϊԭ�� 3���޸�cs��Ϊ�Ѵ��ڵ�cs 4������� 
   */
   var csName1 = CHANGEDPREFIX + "_rename16111_1";
   var csName2 = CHANGEDPREFIX + "_rename16111_2";
   //����cs cl
   commDropCS( db, csName1, true, "ignoreNotExist is true" );
   commDropCS( db, csName2, true, "ignoreNotExist is true" );
   var varCS1 = commCreateCS( db, csName1, true, "create CS" );
   var varCS2 = commCreateCS( db, csName2, true, "create CS" );
   try
   {
      db.renameCS( csName1, csName1 );
   }
   catch( e )
   {
      if( e !== -33 )
      {
         throw buildException( "renameCS( csName1, csName1 ) fail", e, "rename", "success", e );
      }
   }
   try
   {
      db.renameCS( csName1, csName2 );
   }
   catch( e )
   {
      if( e !== -33 )
      {
         throw buildException( "renameCS( csName1, csName1 ) fail", e, "rename", "success", e );
      }
   }
   commDropCS( db, csName1, true, "ignoreNotExist is true" );
   commDropCS( db, csName2, true, "ignoreNotExist is true" );
}