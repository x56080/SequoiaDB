/******************************************************************************
 * @Description   : seqDB-23378:使用数据源创建cs，关联集合创建索引
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23378";
   var csName = "cs_23378";
   var clName = "cl_23378";
   var srcCSName = "datasrcCS_23378";
   var indexName = "index_23378";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "sequoiadb", {ErrorControlLevel:"high"} );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( indexName, { a: 1 }, true, true );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
}
