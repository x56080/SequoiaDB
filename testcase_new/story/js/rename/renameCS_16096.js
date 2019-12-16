/************************************
*@Description: 修改cs名后，检查快照、文件
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16096
**************************************/

main();

function main ()
{
   var oldcsName = COMMCSNAME + "_16096_old";
   var newcsName = COMMCSNAME + "_16096_new";
   var clName = CHANGEDPREFIX + "_16096_cl";
   var lobName = CHANGEDPREFIX + "_16096_lob";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   println( "---create index---" );
   cl.createIndex( "index16096", { age: 1 }, false );

   println( "---insert record---" );
   insertData( cl, 1000 );

   var cmd5 = createFile( lobName );

   println( "---put lob---" );
   var lobArray = putLobs( cl, lobName );

   println( "---test rename cs---" );
   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );
   checkRecord( cl, 1000 );
   checkLob( cl, lobArray, cmd5 );

   var indexArr = ['$id', 'index16096'];
   var cur = cl.listIndexes();
   while( cur.next() )
   {
      var index = cur.current().toObj();
      var name = index.IndexDef.name;
      if( indexArr.indexOf( name ) === -1 )
      {
         throw buildException( "checkIndex", "", "index", indexArr, name );
      }
   }

   deleteFile( lobName );
   commDropCS( db, newcsName, true, false, "clean cs---" );
}

function checkRecord ( dbcl, recordNum )
{
   var actNum = dbcl.count();
   if( actNum != recordNum )
   {
      throw buildException( "checkRecord()", null, "check the new cl record nums",
         recordNum, actNum );
   }
}

