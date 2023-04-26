/******************************************************************************
 * @Description   :seqDB-31197:删除CS，dropCS添加备注后恢复项目。
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.20
 * @LastEditTime  : 2023.04.25
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_31197_1";
   var csName2 = "cs_31197_2";
   var otherCSName = "cs_31197_3";
   var clName = "cl_31197";
   var comment = "dropCS";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   commDropCS( db, otherCSName );
   cleanRecycleBin( db, "cs_31197" );

   commCreateCS( db, csName1 );
   var dbcl1 = commCreateCL( db, csName1, clName );
   commCreateCS( db, csName2 );

   var docs = insertBulkData( dbcl1, 1000 );

   db.dropCS( csName1, { Comment: comment } );
   db.dropCS( csName2, { Comment: comment } );

   var cursor = db.list( SDB_LIST_RECYCLEBIN, { OriginName: csName2 } );
   checkCursorComment( cursor, comment );
   cursor = db.getRecycleBin().list( { OriginName: csName1 } );
   checkCursorComment( cursor, comment );

   cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: csName1 } );
   checkCursorComment( cursor, comment );
   cursor = db.getRecycleBin().snapshot( { OriginName: csName1 } );
   checkCursorComment( cursor, comment )

   var recycleName1 = getOneRecycleName( db, csName1 );
   var recycleName2 = getOneRecycleName( db, csName2 );

   db.getRecycleBin().returnItem( recycleName1 );
   db.getRecycleBin().returnItemToName( recycleName2, otherCSName );

   dbcl1 = db.getCS( csName1 ).getCL( clName );
   cursor = dbcl1.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   db.getCS( otherCSName );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   commDropCS( db, otherCSName );
   cleanRecycleBin( db, "cs_31197" );
}