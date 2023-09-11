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
   // insertOne
   cl.insertOne( { "_id": 11, "a": Timestamp() } );
   cl.insertOne( { "_id": 12, "a": new Timestamp() } );
   cl.insertOne( { "_id": 13, "a": new Timestamp( 0, 0 ) } );
   // insertMany
   cl.insertMany( [{ "_id": 21, "a": Timestamp() }, { "_id": 22, "a": new Timestamp() }, { "_id": 23, "a": new Timestamp( 0, 0 ) }] );
   // save
   cl.save( { "_id": 31, "a": Timestamp() } );
   cl.save( { "_id": 32, "a": new Timestamp() } );
   cl.save( { "_id": 33, "a": new Timestamp( 0, 0 ) } );
   // 检查结果
   assert.eq( cl.count(), 12 );
   var rc = cl.find( {}, { "_id": 0, "a": 1 } ).sort( { "_id": 1 } );
   while( rc.hasNext() )
   {
      var doc = rc.next();
      // 当前时间实时变化，校验结果只对比实际秒数并容许300秒误差值
      // 3.2及以下版本，JSON.stringify( doc ) = {"a":{"$timestamp":{"t":1629091200,"i":0}}}
      var aTimeS = doc.a.t;
      assert.lte( Math.abs( currentTimeS - aTimeS ), 300, JSON.stringify( doc ) );
   }
   rc.close();


   // 更新数据，update Timestamp()，同原生mongo，返回Timestamp(0, 0)
   cl.remove( {} );
   cl.insert( [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 2, "a": 1, "b": 1 }, { "_id": 3, "a": 1, "b": 1 }] );
   // update 
   cl.update( { "_id": 1 }, { $set: { "a": Timestamp(), "b": new Timestamp() } } );
   // updateOne
   cl.updateOne( { "_id": 2 }, { $set: { "a": Timestamp(), "b": new Timestamp() } } );
   // updateMany 
   cl.updateMany( { "_id": 3 }, { $set: { "a": Timestamp(), "b": new Timestamp() } } );
   // 检查结果
   assert.eq( cl.count( { "a": Timestamp( 0, 0 ), "b": Timestamp( 0, 0 ) } ), 3 );


   // 删除数据
   // remove
   cl.remove( { "_id": 1, "a": Timestamp(), "b": new Timestamp() } );
   // deleteOne
   cl.deleteOne( { "_id": 2, "a": Timestamp(), "b": new Timestamp() } );
   // deleteMany
   cl.deleteMany( { "_id": 3, "a": Timestamp(), "b": new Timestamp() } );
   // 检查结果
   assert.eq( cl.count(), 0 );


   cl.drop();
}