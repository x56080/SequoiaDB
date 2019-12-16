/************************************
*@Description: 修改cs名后，执行数据增删改查操作---//review 1:描述和实际执行步骤不相符
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16102
**************************************/

main();

function main ()
{
   var oldcsName = CHANGEDPREFIX + "_16102_oldcs";
   var newcsName = CHANGEDPREFIX + "_16102_newcs";
   var clName = CHANGEDPREFIX + "_16102_cl";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   cl.createIndex( "aIndex", { a: 1 }, true );
   cl.createIndex( "noIndex", { no: 1 }, false );

   //insert 1000 data
   insertData( cl, 1000 );

   println( "---rename cs---" );
   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );

   cl.dropIndex( "aIndex" );
   cl.dropIndex( "noIndex" );

   cl.createIndex( "aIndex", { a: 1 }, true );
   cl.createIndex( "noIndex", { no: 1 }, false );

   var indexArr = ['$id', 'aIndex', 'noIndex'];

   var cur = cl.listIndexes();
   while( cur.next() )
   {
      var index = cur.current().toObj();
      var name = index.IndexDef.name;
      if( indexArr.indexOf( name ) === -1 )// review 3:这种校验索引的方法不严谨，如果查不到索引或者索引不足3个，那么这种方法就无法检测到
      {
         throw buildException( "checkIndex", e, "index", indexArr.toString, name );
      }
   }

   checkRenameCSResult( oldcsName, newcsName, 1 );

   commDropCS( db, newcsName, true, false, "clean cs---" );
}
