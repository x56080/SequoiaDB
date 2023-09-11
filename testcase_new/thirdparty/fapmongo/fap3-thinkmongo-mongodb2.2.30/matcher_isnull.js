/**
 * @description mongo thinkjs
 * @testcase seqDB-33073:CRUD/aggregate操作，匹配条件为{a:null}
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

      let rc;

      // 准备数据
      let nullDocs = [{ "_id": 1 }, { "_id": 2, "a": null }];
      let notnullDocs = [{ "_id": 3, "a": 2 }];
      let docs = nullDocs.concat( notnullDocs );
      await m.addMany( docs );

      // find，返回单条记录
      rc = await m.find( { "a": null } );
      Assert.equal( JSON.stringify( rc ), '{"_id":1}' );

      // select
      // {a:null}
      rc = await m.order( { "_id": 1 } ).where( { "a": null } ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( nullDocs ) );
      //{a:{$ne:null}}
      rc = await m.order( { "_id": 1 } ).where( { "a": { "$ne": null } } ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( notnullDocs ) );

      // agregate
      rc = await m.aggregate( { "$match": { "a": null } } );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( nullDocs ) );

      // update
      rc = await m.where( { "a": null } ).update( { "u1": 1 } );
      Assert.equal( JSON.stringify( rc ), 2 );
      // thenUpdate
      rc = await m.update( { "u2": 1 }, { "a": null } );
      Assert.equal( JSON.stringify( rc ), 3 );
      // updateMany
      rc = await m.where( { "a": null } ).updateMany( [{ "u3": 1 }], {} );
      Assert.equal( JSON.stringify( rc ), 2 );
      // check result      
      rc = await m.order( { "_id": 1 } ).where( {} ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"u1":1,"u2":1,"u3":1},{"_id":2,"a":null,"u1":1,"u2":1,"u3":1},{"_id":3,"a":2,"u2":1}]' );

      // delete
      rc = await m.where( { "a": null } ).delete();
      Assert.equal( rc, 2 );
      // check result      
      rc = await m.order( { "_id": 1 } ).where( {} ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":3,"a":2,"u2":1}]' );

      await m.delete();

      console.log( "---End test." );

      this.assign( "m", await m.select() );
      return this.display();
   }

};
