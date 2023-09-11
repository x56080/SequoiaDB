/******************************************************************************
 * @Description   : seqDB-32902:insert* / save / update* / delete* timestamp()
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_32902";
   var cl = db.getCollection( clName );
   cl.drop();

   // 获取当前时间秒数
   var currentDate = new Date();
   var currentTimeMS = currentDate.getTime();
   var currentTimeS = Math.floor( currentTimeMS / 1000 );

   // new Timestamp(), 同原生mongo，返回Timestamp(0, 0)
   var rc = new Timestamp();
   assert.eq( rc, Timestamp( 0, 0 ) );

   // 写数据，insert Timestamp()，同原生mongo，转换为当前时间
   // insert
   cl.insert( [{ "_id": 1, "a": Timestamp() }, { "_id": 2, "a": new Timestamp() }, { "_id": 3, "a": new Timestamp( 0, 0 ) }] );
   // save
   cl.save( { "_id": 31, "a": Timestamp() } );
   cl.save( { "_id": 32, "a": new Timestamp() } );
   cl.save( { "_id": 33, "a": new Timestamp( 0, 0 ) } );
   // 检查结果
   assert.eq( cl.count(), 6 );
   var rc = cl.find( {}, { "_id": 0, "a": 1 } ).sort( { "_id": 1 } );
   while( rc.hasNext() )
   {
      var doc = rc.next();
      // 当前时间实时变化，校验结果只对比实际秒数并容许300秒误差值
      // 3.2及以下版本，JSON.stringify( doc ) = {"a":{"$timestamp":{"t":1629091200,"i":0}}}
      var aTimeS = doc.a.t;
      assert.lte( Math.abs( currentTimeS - aTimeS ), 300, JSON.stringify( doc ) );
   }


   // 更新数据，update Timestamp()，同原生mongo，返回Timestamp(0, 0)
   cl.remove( {} );
   cl.insert( [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 2, "a": 1, "b": 1 }, { "_id": 3, "a": 1, "b": 1 }] );
   // update 
   cl.update( { "_id": 1 }, { $set: { "a": Timestamp(), "b": new Timestamp() } } );
   // 检查结果
   assert.eq( cl.count( { "a": Timestamp( 0, 0 ), "b": Timestamp( 0, 0 ) } ), 1 );


   // 删除数据
   // remove
   cl.remove( { "_id": 1, "a": Timestamp(), "b": new Timestamp() } );
   // 检查结果
   assert.eq( cl.count(), 2 );


   cl.drop();
}