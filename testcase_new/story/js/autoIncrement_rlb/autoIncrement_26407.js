/******************************************************************************
 * @Description   : seqDB-26407:包含自增字段的集合插入数据，catalog切主后再次插入数据
 * @Author        : liuli
 * @CreateTime    : 2022.04.20
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_26407";

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   dbcl.createAutoIncrement( { Field: "test", CacheSize: 1, AcquireSize: 1 } )

   // 插入一些数据
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );

   // 新建一个连接
   db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var rg = db2.getCataRG();
   rg.reelect();

   // 再次插入数据
   dbcl.insert( docs );

   db2.close();
   commCheckBusinessStatus( db );
}
