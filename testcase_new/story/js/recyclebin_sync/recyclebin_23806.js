/******************************************************************************
 * @Description   : seqDB-23806:分区表且被切分到多个数据组，读写数据并snapshot查看集合统计信息，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2022.03.04
 * @LastEditTime  : 2024.06.20
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23806";
   var clName = "cl_23806";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true, ReplSize: 0 } );

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   commCheckLSN( db );
   var expCursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, RawData: true },
      { "Details.UpdateTime": { "$include": 0 },
        "Details.DataCommitLSN": { "$include": 0 } },
      { "Details.NodeName": 1 } );

   dbcs.dropCL( clName );

   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   commCheckLSN( db );
   var actCursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, RawData: true },
      { "Details.UpdateTime": { "$include": 0 },
        "Details.DataCommitLSN": { "$include": 0 } },
      { "Details.NodeName": 1 } );

   if ( actCursor.size() != expCursor.size() )
   {
      throw error( "Invalid snapshot cl result size[ act: " + actCursor.size() +
                   ", exp: " + expCursor.size() + " ]" );
   }

   while( actCursor.next() && expCursor.next() )
   {
      assert.equal( actCursor.current().toObj(), expCursor.current().toObj() );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}
