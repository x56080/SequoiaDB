/******************************************************************************
 * @Description   : seqDB-30937:通过快照/列表查看RecycleTime字段类型
 * @Author        : Bi Qin
 * @CreateTime    : 2023.03.25
 * @LastEditTime  : 2023.03.31
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30937";
   var clName = "cl_30937";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   dbcs.createCL( clName );
   dbcs.dropCL( clName );

   var cursor = db.list( SDB_LIST_RECYCLEBIN, {}, { "RecycleTime": { "$type": 2 } } );
   checkCursor( cursor );

   cursor = db.getRecycleBin().list( {}, { "RecycleTime": { "$type": 2 } } );
   checkCursor( cursor );

   cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { Role: "data", RawData: true }, { "RecycleTime": { "$type": 2 } } );
   checkCursor( cursor );

   cursor = db.getRecycleBin().snapshot( { Role: "data", RawData: true }, { "RecycleTime": { "$type": 2 } } );
   checkCursor( cursor );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function checkCursor ( cursor )
{
   assert.equal( cursor.current().toObj().RecycleTime, "string" );
   cursor.close();
}