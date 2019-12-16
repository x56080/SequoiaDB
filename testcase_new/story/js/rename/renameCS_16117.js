/* *****************************************************************************
@discretion: rename cl
             seqDB-16117
@author��2018-10-13 chensiqin  Init
***************************************************************************** */

main( db );
function main ( db )
{
   /*
     1������cs�Ͷ��cl���ֱ���cl�ϴ���������� 
     2���޸�cs����ִ����������������� 
     check:
     1���޸�cs���ɹ� 
     2���޸���ɺ���ɾ�����ɹ�
   */
   var csName1 = CHANGEDPREFIX + "_cs16117_1";
   var csName2 = CHANGEDPREFIX + "_cs16117_2";
   var clName1 = CHANGEDPREFIX + "_cl16117_1";
   var clName2 = CHANGEDPREFIX + "_cl16117_2";
   var clName3 = CHANGEDPREFIX + "_cl16117_3";
   var indexName1 = "index16117_1";
   var indexName2 = "index16117_2";
   var indexName3 = "index16117_3";
   commDropCS( db, csName1, true, "drop CS " + csName1 );
   commDropCS( db, csName2, true, "drop CS " + csName2 );
   var cs = commCreateCS( db, csName1, true, "create CS1" );
   var cl1 = commCreateCL( db, csName1, clName1, {}, true, false, "create cl in the beginning" );
   var cl2 = commCreateCL( db, csName1, clName2, {}, true, false, "create cl in the beginning" );
   var cl3 = commCreateCL( db, csName1, clName3, {}, true, false, "create cl in the beginning" );

   commCreateIndex( cl1, indexName1, { a: 1 }, false, false );
   commCreateIndex( cl1, indexName2, { b: 1 }, false, false );
   commCreateIndex( cl2, indexName1, { a: 1 }, false, false );
   commCreateIndex( cl2, indexName2, { b: 1 }, false, false );
   commCreateIndex( cl3, indexName1, { a: 1 }, false, false );
   commCreateIndex( cl3, indexName2, { b: 1 }, false, false );
   try
   {
      db.renameCS( csName1, csName2 );
   }
   catch( e )
   {
      throw buildException( "renameCS( csName1, csName2 ) fail", e, "rename", "success", e );
   }

   checkRenameCSResult( csName1, csName2, 3 );

   var cs = db.getCS( csName2 );
   var cl1 = cs.getCL( clName1 );
   var cl2 = cs.getCL( clName2 );
   var cl3 = cs.getCL( clName3 );
   cl1.dropIndex( indexName2 );
   cl2.dropIndex( indexName2 );
   cl3.dropIndex( indexName2 );
   commCreateIndex( cl1, indexName3, { c: 1 }, false, false );
   commCreateIndex( cl2, indexName3, { c: 1 }, false, false );
   commCreateIndex( cl3, indexName3, { c: 1 }, false, false );
   checkClIndex( cl1, indexName1, indexName3 );
   checkClIndex( cl2, indexName1, indexName3 );
   checkClIndex( cl3, indexName1, indexName3 );

   commDropCS( db, csName1, true, "drop CS " + csName1 );
   commDropCS( db, csName2, true, "drop CS " + csName2 );
}

function checkClIndex ( cl, indexName1, indexName3 )
{
   var indexInfos = cl.listIndexes();
   var cnt = 0;
   var myIndexNum = 0;
   while( indexInfos.next() )
   {
      cnt++;
      var ret = indexInfos.current();
      var indexInfo = ret.toObj()["IndexDef"];
      var indeName = indexInfo["name"];
      if( indeName == indexName1 || indeName == indexName3 )
      {
         myIndexNum++;
      }
   }
   indexInfos.close();
   if( cnt !== 3 )
   {
      throw buildException( "check cl.listIndexes() num fail", "fail", "check", 3, cnt );
   }
   if( myIndexNum !== 2 )
   {
      throw buildException( "check expected indexnum fail", "fail", "check", 2, cnt );
   }
}