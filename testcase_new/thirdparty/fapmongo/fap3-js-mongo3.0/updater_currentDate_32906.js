/******************************************************************************
 * @Description   : seqDB-32906:update使用$currentDate操作符
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_32906";
   var cl = db.getCollection( clName );
   cl.drop();
   cl.insert( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }, { "_id": 4, "a": 4 }, { "_id": 5, "a": 5 }] );

   // 获取当前时间毫秒数
   var currentDate = new Date();
   var currentTimeMS = currentDate.getTime();

   // update
   var rc = cl.update( { "_id": { "$in": [2, 3] } }, { "$currentDate": { "a": true } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );

   // check results
   var rc = cl.find( { "_id": { "$in": [2, 3] } } );
   while( rc.hasNext() )
   {
      var actTimeMS = rc.next().a.getTime();
      // 当前时间实时变化，校验结果只对比实际秒数并容许600秒误差值（结果同原生mongo引擎）
      assert.lte( Math.abs( currentTimeMS - actTimeMS ), 600 * 1000 );
   }

   cl.drop();
}