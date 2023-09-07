/******************************************************************************
 * @Description   : seqDB-33204:aggregate() / aggregate([]) / aggregate({})
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.09.07
 * @LastEditTime  : 2023.09.07
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33204";
   var cl = db.getCollection( clName );
   cl.drop();
   var docs = [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }];
   cl.insert( docs );

   // aggregate()
   var rc = cl.aggregate();
   checkResults( rc, JSON.stringify( docs ) );

   // aggregate([])
   var rc = cl.aggregate( [] );
   checkResults( rc, JSON.stringify( docs ) );

   // aggregate({})，3.4以下版本执行报错内部直接assert出去了，无法try catch
   // 此异常场景在 4.0 版本不关注

   cl.drop();
}

function checkResults ( rc, expDocs )
{
   var docs = new Array();
   while( rc.hasNext() )
   {
      var doc = rc.next();
      docs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( docs.sort() ), expDocs );
}