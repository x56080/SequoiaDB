/******************************************************************************
 * @Description   :seqDB-31200:删除重复集合空间，备注不同
 *                 seqDB-31201:dropCL删除重复集合，备注不同
                   seqDB-31202:truncate删除重复集合，备注不同
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.21
 * @LastEditTime  : 2023.04.24
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_31200_31201_31202_1";
   var csName2 = "cs_31200_31201_31202_2";
   var clName1 = "cl_31200";
   var clName2 = "cl_31201";
   var clName3 = "cl_31202";
   var comments = ["31200_31201_31202_1", "31200_31201_31202_2"];

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_31200_31201_31202_" );

   commCreateCS( db, csName1 );
   var dbcl1 = commCreateCL( db, csName1, clName1 );
   var docs1 = insertBulkData( dbcl1, 1000 );

   var dbcs2 = commCreateCS( db, csName2 );
   var dbcl2 = commCreateCL( db, csName2, clName2 );
   var dbcl3 = commCreateCL( db, csName2, clName3 );

   insertBulkData( dbcl2, 2000 );
   insertBulkData( dbcl3, 3000 );
   //seqDB-31200:删除重复集合空间，备注不同
   db.dropCS( csName1, { Comment: comments[0] } );

   commCreateCS( db, csName1 );
   dbcl1 = commCreateCL( db, csName1, clName1 );
   insertBulkData( dbcl1, 1000 );

   db.dropCS( csName1, { Comment: comments[1] } );

   var recycleNames = getRecycleName( db, csName1 );
   checkCursorByRecycleNames( recycleNames, comments );
   //seqDB-31201:dropCL删除重复集合，备注不同
   dbcs2.dropCL( clName2, { Comment: comments[0] } );

   dbcl2 = commCreateCL( db, csName2, clName2 );
   insertBulkData( dbcl2, 2000 );

   dbcs2.dropCL( clName2, { Comment: comments[1] } );

   recycleNames = getRecycleName( db, csName2 + "." + clName2 );
   checkCursorByRecycleNames( recycleNames, comments );
   //seqDB-31202:truncate删除重复集合，备注不同
   dbcl3.truncate( { Comment: comments[0] } );

   dbcl3.truncate( { Comment: comments[1] } );

   recycleNames = getRecycleName( db, csName2 + "." + clName3 );
   checkCursorByRecycleNames( recycleNames, comments );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_31200_31201_31202_" );
}

function checkCursorByRecycleNames ( recycleNames, comments )
{
   var cursor;
   for( var i = 0; i < recycleNames.length; i++ )
   {
      cursor = db.list( SDB_LIST_RECYCLEBIN, { RecycleName: recycleNames[i] } );
      checkCursorComment( cursor, comments[i] );
      cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { RecycleName: recycleNames[i] } );
      checkCursorComment( cursor, comments[i] );
   }
   cursor.close();
}