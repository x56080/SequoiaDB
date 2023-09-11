/**
 * @description  seqDB-33076:update使用$currentDate操作符
 * @author XiaoNi Huang 2023-08-23
 * @note 每次跑用例之前需要确认：
 *    1、cs已创建；
 *    2、cl不存在/cl不存在唯一索引，避免插入数据索引键冲突。由于think-mongo没有删除cl/index接口，需要人工干预手工处理
 */

const Base = require( "./base.js" );
const Assert = require( "assert" );

module.exports = class extends Base
{
   async indexAction ()
   {
      console.log( "---Begin test." );

      const m = this.mongo( "cl_think_mongo" );
      let db = m.db();
      // 插入记录，自动创建cs/cl，创建完成后清空cl
      await m.add( { "beforeTest": 1 } );
      await m.delete();

      // 准备数据
      let docs = [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }];
      await m.addMany( docs );

      let rc;

      // update
      rc = await m.where( { "_id": 1 } ).update( { "$currentDate": { "a": true } } );
      Assert.equal( rc, 1 );
      // check result
      rc = await m.where( { "a": 1 } ).select();
      Assert.equal( rc, 0 );
      console.log( await m.where( { "_id": 1 } ).select() );


      // updateMany
      rc = await m.where( { "_id": 2 } ).updateMany( [{ "$currentDate": { "a": true } }] );
      Assert.equal( rc, 1 );
      // check result
      rc = await m.where( { "a": 2 } ).select();
      Assert.equal( rc, 0 );
      console.log( await m.where( { "_id": 2 } ).select() );

      await m.delete();

      console.log( "---End test." );

      this.assign( "m", await m.select() );
      return this.display();
   }

};
