/******************************************************************************
 * @Description   :seqDB-31198:删除CL，dropCL添加备注后恢复项目
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.20
 * @LastEditTime  : 2023.04.24
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_31198";
   var clName1 = "cl_31198_1";
   var clName2 = "cl_31198_2";
   var otherCLName = "cl_31198_3";
   var comment = "dropCL";
   var fullName1 = csName + "." + clName1;
   var fullName2 = csName + "." + clName2;
   var fullName3 = csName + "." + otherCLName;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl1 = commCreateCL( db, csName, clName1 );
   var dbcl2 = commCreateCL( db, csName, clName2 );

   var docs1 = insertBulkData( dbcl1, 1000 );
   var docs2 = insertBulkData( dbcl2, 2000 );

   dbcs.dropCL( clName1, { Comment: comment } );
   dbcs.dropCL( clName2, { Comment: comment } );

   var cursor = db.list( SDB_LIST_RECYCLEBIN, { OriginName: fullName2 } );
   checkCursorComment( cursor, comment );
   cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: fullName1 } );
   checkCursorComment( cursor, comment );

   var recycleName1 = getOneRecycleName( db, fullName1 );
   var recycleName2 = getOneRecycleName( db, fullName2 );

   db.getRecycleBin().returnItem( recycleName1 );
   db.getRecycleBin().returnItemToName( recycleName2, fullName3 );

   dbcl1 = dbcs.getCL( clName1 );
   var dbcl3 = dbcs.getCL( otherCLName );
   cursor = dbcl1.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );
   cursor = dbcl3.find().sort( { "a": 1 } );
   commCompareResults( dbcl3.find(), docs2 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}