/******************************************************************************
 * @Description   : seqDB-23825:分区表且被切分到多个数据组，读写数据并snapshot查看集合统计信息，truncate后恢复
 * @Author        : liuli
 * @CreateTime    : 2022.03.04
 * @LastEditTime  : 2024.07.01
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23825";
   var clName = "cl_23825";

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

   checkRecycleRecover( dbcs, dbcl, csName, clName, "Truncate" ) ;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}