/**
 * @description seqDB-33077:aggregate使用$unwind操作符
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
      let docs = [
         { "_id": 1, "a": [1, 2], "c": 1 },
         { "_id": 2, "a": [2, 3], "c": 2 },
         { "_id": 3, "a": [3], "c": 3 },
         { "_id": 4, "a": [], "c": 4 },
         { "_id": 5, "a": [{ "a1": [1, 2] }, { "a1": [3, 4] }], "c": 5 },
         { "_id": 6, "a": 6, "c": 6 },
         { "_id": 7, "a": null, "c": 7 },
         { "_id": 8, "c": 8 }
      ];
      await m.addMany( docs );

      let rc;

      // test
      rc = await m.aggregate( [{ "$unwind": { "path": "$a", "preserveNullAndEmptyArrays": true } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"c":1},{"_id":1,"a":2,"c":1},{"_id":2,"a":2,"c":2},{"_id":2,"a":3,"c":2},{"_id":3,"a":3,"c":3},{"_id":4,"a":null,"c":4},{"_id":5,"a":{"a1":[1,2]},"c":5},{"_id":5,"a":{"a1":[3,4]},"c":5},{"_id":6,"a":6,"c":6},{"_id":7,"a":null,"c":7},{"_id":8,"c":8}]' );

      await m.delete();

      console.log( "---End test." );

      this.assign( "m", await m.select() );
      return this.display();
   }

};
