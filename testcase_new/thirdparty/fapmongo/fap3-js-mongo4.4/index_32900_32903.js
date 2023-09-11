/******************************************************************************
 * @Description   : seqDB-32900:getIndexes获取索引时集合空间/集合不存在
 *    seqDB-32903:createIndex，name带非法字符"."
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_32900";
   var cl = db.getCollection( clName );
   cl.drop();

   // seqDB-32900
   // mongo cs 不存在，getIndexes
   db.dropDatabase();
   var rc = cl.getIndexes();
   assert.eq( rc, "" );
   // mongo cl 不存在，getIndexes
   cl.insert( { "a": 1 } );
   cl.drop();
   var rc = cl.getIndexes();
   assert.eq( rc, "" );
   // sequoiadb cs/cl不存在在 JAVA TESTNG 测试框架覆盖


   // seqDB-32903
   // createIndex, name带“.” ( fapmongo会将"."转换为"%2E" )
   var idxName = ".a.idx.";
   var rc = cl.createIndex( { a: 1 }, { "name": idxName } );
   assert.eq( rc, { "ok": 1 } );
   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc.sort() ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"" + idxName + "\",\"ns\":\"" + cl.toString() + "\"}]" );
   // dropIndex
   var rc = cl.dropIndex( idxName );
   assert.eq( rc, { "ok": 1 } );
   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );

   // createIndex, name 包含"%2E"
   var idxName = "a%2Eb";
   var rc = cl.createIndex( { a: 1 }, { "name": idxName } );
   assert.eq( rc, { "ok": 1 } );
   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"a.b\",\"ns\":\"" + cl.toString() + "\"}]" );
   // dropIndex
   var rc = cl.dropIndex( idxName );
   assert.eq( rc, { "ok": 1 } );
   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );

   cl.drop();
}