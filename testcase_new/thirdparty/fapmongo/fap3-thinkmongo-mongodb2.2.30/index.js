/**
 * @description mongo thinkjs
 * @testcase sequoiaDB / Story测试 / 兼容mongodb / mongodb驱动测试 / thinkjs/nodejs驱动
 * @author XiaoNi Huang 2020-03-01
 * @modify XiaoNi Huang 2020-08-20
 * @note 每次跑用例之前需要确认：
 *     1、cs已创建；
 *     2、cl不存在/cl不存在唯一索引，避免插入数据索引键冲突。由于think-mongo没有删除cl/index接口，需要人工干预手工处理
 */

const Base = require( "./base.js" );
const Assert = require( "assert" );

module.exports = class extends Base
{
   // thinkjs没有创建cs/cl的接口，没有删除索引的接口，用例执行前只能自动创建，用例执行后需要手工删除集合/索引

   async indexAction ()
   {
      const m = this.mongo( "cl_think_mongo" );
      let db = m.db();
      // 插入记录，自动化创建cs/cl，创建完成后清空cl
      await m.add( { "beforeTest": 1 } );
      await m.delete();

      const clName = "cs_think_mongo.cl_think_mongo";
      let docs;
      let expDocs;
      let rc;


      // testcase: seqDB-21813:增删改查数据 

      // model.add( data, options )
      // a.指定data添加数据
      rc = await m.add( { "_id": 1, "a": 1, "b": "add" } );
      Assert.equal( rc, "1" );
      // b.指定data和options添加数据
      rc = await m.add( { "_id": 2, "a": 2, "b": "add" } );
      Assert.equal( rc, "2" );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"add"},{"_id":2,"a":2,"b":"add"}]' );

      // model.thenAdd( data, where )
      // a.指定data和where添加数据，where未命中数据
      rc = await m.thenAdd( { "_id": 11, "a": 11, "b": "thenAdd" }, { "a": 11 } );
      Assert.equal( JSON.stringify( rc ), '{"_id":"11","type":"add"}' );
      // b.指定data和where添加数据，where命中数据
      // 实际返回如：{"_id":1,"type":"exist"}，返回_id随机，只校验type
      rc = await m.thenAdd( { "_id": 12, "a": 12, "b": "thenAdd" }, { "a": 11 } );
      Assert.equal( rc.type, 'exist' );
      // c.指定data添加数据
      rc = await m.thenAdd( { "_id": 13, "a": 13, "b": "thenAdd" } );
      Assert.equal( rc.type, 'exist' );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).where( { "a": { "$gt": 10 } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":11,"a":11,"b":"thenAdd"}]' );

      // model.addMany( dataList, options )
      // a.指定dataList添加多条记录，addMany只指定dataList，数据包含主键的值
      docs = [
         { "_id": 21, "a": 21, "b": "addMany" },
         { "_id": 22, "a": 22, "b": "addMany" }
      ];
      rc = await m.addMany( docs );
      Assert.equal( JSON.stringify( rc ), '["21","22"]' );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).where( { "a": { "$gt": 20 } } ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs ) );
      // b.指定dataList添加记录，指定dataList和options，数据不包含主键的值
      docs = [
         { "a": 23, "b": "addMany" },
         { "a": 24, "b": "addMany" }
      ];
      let rcAutoids = await m.addMany( docs, { "limit": 1 } );
      Assert.equal( rc.length, docs.length );
      // 检查结果
      expDocs = [
         { "a": 23, "b": "addMany", "_id": rcAutoids[0] },
         { "a": 24, "b": "addMany", "_id": rcAutoids[1] }
      ];
      rc = await m.order( { "_id": 1 } ).where( { "a": { "$gt": 22 } } ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( expDocs ) );

      // model.where( query ).update( data ), 返回更新成功的记录数
      // a.指定data更新数据；跟where组合使用，指定where匹配更新部分数据
      rc = await m.where( { "a": { "$lte": 10 } } ).update( { "b": "update1" } );
      Assert.equal( rc, 2 );
      // b.指定data更新数据；跟where组合使用，指定where为1=1匹配所有数据  ---TODO 用法有问题，暂不关注
      //rc = await m.where( '1=1' ).update( { "t": "update2" } );
      //Assert.equal( rc, 9 ); 
      // c.指定data和options更新数据
      rc = await m.where( { "a": { "$gt": 10 } } ).update( { "b": "update2" }, { "test": 1 } );
      Assert.equal( rc, 5 );
      // d.单独使用update(data)更新数据
      rc = await m.update( { "c": "update3" } );
      Assert.equal( rc, 7 );
      // 检查结果，带options，model.select( options )
      // and check results after update
      rc = await m.order( { "_id": 1 } ).select( { "a": { "$exists": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"update1","c":"update3"},{"_id":2,"a":2,"b":"update1","c":"update3"},{"_id":11,"a":11,"b":"update2","c":"update3"},{"_id":21,"a":21,"b":"update2","c":"update3"},{"_id":22,"a":22,"b":"update2","c":"update3"},{"_id":"' + rcAutoids[0] + '","a":23,"b":"update2","c":"update3"},{"_id":"' + rcAutoids[1] + '","a":24,"b":"update2","c":"update3"}]' );

      // model.thenUpdate( data, where ), 返回_id
      // thenUpdate不指定where，则更新所有数据；
      // 当where条件命中数据时更新数据；当where条件未命中到任何数据时才添加数据
      // a.指定data添加数据，未指定where，随机更新任意1条数据。
      let rcId = await m.thenUpdate( { "b": "thenUpdate" } );
      if( rcId === "undefined" )
      {
         Assert.fail( "return _id fail" );
      }
      // 检查结果
      rc = await m.order( { "a": 1 } ).where( { "b": "thenUpdate" } ).select();
      Assert.equal( rc.length, 7 );

      // b.指定data和where添加数据，where未命中数据
      rc = await m.thenUpdate( { "_id": 31, "a": 31, "b": "thenUpdate" }, { "test": "notExist" } );
      Assert.equal( rc, 31 );
      // c.指定data和where添加数据，where命中数据
      rc = await m.thenUpdate( { "b": "thenUpdate" }, { "a": 1 } );
      Assert.equal( rc, 1 );
      // d.thenUpdate(data)跟where()方法组合使用，where未命中记录
      // model.where( query ).thenUpdate( data, where )
      rc = await m.where( { "test": "notExist" } ).thenUpdate( { "_id": 32, "a": 32, "b": "thenUpdate" } );
      Assert.equal( rc, 32 );

      // 检查结果
      rc = await m.order( { "a": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"thenUpdate","c":"update3"},{"_id":2,"a":2,"b":"thenUpdate","c":"update3"},{"_id":11,"a":11,"b":"thenUpdate","c":"update3"},{"_id":21,"a":21,"b":"thenUpdate","c":"update3"},{"_id":22,"a":22,"b":"thenUpdate","c":"update3"},{"_id":"' + rcAutoids[0] + '","a":23,"b":"thenUpdate","c":"update3"},{"_id":"' + rcAutoids[1] + '","a":24,"b":"thenUpdate","c":"update3"},{"_id":31,"a":31,"b":"thenUpdate"},{"_id":32,"a":32,"b":"thenUpdate"}]' );

      // model.updateMany( dataList, options ), 返回更新的记录数 
      // a.指定dataList更新多条记录，数据包含主键的值
      docs = [
         { "_id": 1, "b": "updateMany1" },
         { "_id": 2, "b": "updateMany1" }
      ];
      rc = await m.updateMany( docs );
      Assert.equal( rc, 2 );
      // b.跟where组合使用，指定dataList和options更新多条记录，数据包含主键的值
      docs = [
         { "_id": 1, "c": "updateMany2" },
         { "_id": 2, "c": "updateMany2" }
      ];
      rc = await m.where( { "a": { "$gt": 1 } } ).updateMany( docs, { 't': "notExist" } );
      Assert.equal( rc, 1 );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).where( { "_id": { "$lt": 3 } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"updateMany1","c":"update3"},{"_id":2,"a":2,"b":"updateMany1","c":"updateMany2"}]' );
      // c.不包含主键更新
      docs = [
         { "b": "updateMany3" }
      ];
      rc = await m.updateMany( docs );
      Assert.equal( rc, 9 );
      // d.跟where组合使用，不包含主键更新
      docs = [
         { "c": "updateMany4" }
      ];
      rc = await m.where( { "a": { "$lte": 10 } } ).updateMany( docs );
      Assert.equal( rc, 2 );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).where().select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"updateMany3","c":"updateMany4"},{"_id":2,"a":2,"b":"updateMany3","c":"updateMany4"},{"_id":11,"a":11,"b":"updateMany3","c":"update3"},{"_id":21,"a":21,"b":"updateMany3","c":"update3"},{"_id":22,"a":22,"b":"updateMany3","c":"update3"},{"_id":31,"a":31,"b":"updateMany3"},{"_id":32,"a":32,"b":"updateMany3"},{"_id":"' + rcAutoids[0] + '","a":23,"b":"updateMany3","c":"update3"},{"_id":"' + rcAutoids[1] + '","a":24,"b":"updateMany3","c":"update3"}]' );

      // model.delete( options )
      // a.带条件删除部分记录
      rc = await m.where( { "a": { "$gte": 10 } } ).delete( { "t": "not exist" } );
      Assert.equal( rc, 7 );
      // 检查结果
      rc = await m.order( { "_id": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":"updateMany3","c":"updateMany4"},{"_id":2,"a":2,"b":"updateMany3","c":"updateMany4"}]' );

      // find(options)
      // 不带options
      rc = await m.order( { "_id": 1 } ).find();
      Assert.equal( JSON.stringify( rc ), '{"_id":1,"a":1,"b":"updateMany3","c":"updateMany4"}' );
      // 带options
      rc = await m.order( { "_id": -1 } ).find( { "t": 1 } );
      Assert.equal( JSON.stringify( rc ), '{"_id":2,"a":2,"b":"updateMany3","c":"updateMany4"}' );

      // model.delete()
      rc = await m.delete();
      Assert.equal( rc, 2 );
      // 检查结果 
      rc = await m.select();
      Assert.equal( JSON.stringify( rc ), '[]' );

      // 无效参数测试
      try
      {
         await m.update( { "$inv": { "a": 1000 } } );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         // 报错Updator operator[$inv] error
         if( e.message !== 'Updator operator[$inv] error' )
         {
            throw e;
         }
      }

      // clean data
      await m.delete();


      // testcase: seqDB-21816:自增/自减字段  

      // model.increment( field, step )
      // 准备数据
      docs = [
         { "_id": 1, "a": 1, "b": 1, "c": 1 },
         { "_id": 2, "a": 2, "b": 1, "c": 1 }
      ];
      await m.addMany( docs );
      // 字段存在
      rc = await m.where( { "a": 1 } ).increment( "b", 1 );
      Assert.equal( rc, 1 );
      // 字段不存在
      rc = await m.where( { "a": 1 } ).increment( "t1", 1 );
      Assert.equal( rc, 1 );
      // step参数值为null
      try
      {
         await m.where( { "a": 2 } ).increment( "c", "test" );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -6 )
         {
            Assert.fail( "expect e.code = -6, actual e.code = " + e.code );
         }
      }
      // step参数值为string
      try
      {
         await m.where( { "a": 2 } ).increment( "c", "test" );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -6 )
         {
            Assert.fail( "expect e.code = -6, actual e.code = " + e.code );
         }
      }
      // 检查结果
      rc = await m.order( { "a": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":2,"c":1,"t1":1},{"_id":2,"a":2,"b":1,"c":1}]' );

      // model.decrement( field, step )
      // 字段存在
      rc = await m.where( { "a": 1 } ).decrement( "b", -2 );
      Assert.equal( rc, 1 );
      // 字段不存在
      rc = await m.where( { "a": 1 } ).decrement( "t2", -2 );
      Assert.equal( rc, 1 );
      // step参数值为null
      rc = await m.where( { "a": 2 } ).decrement( "b", null );
      Assert.equal( rc, 1 );
      // step参数值为string
      rc = await m.where( { "a": 2 } ).decrement( "c", "test" );
      Assert.equal( rc, 1 );
      // 检查结果
      rc = await m.order( { "a": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":4,"c":1,"t1":1,"t2":2},{"_id":2,"a":2,"b":1,"c":null}]' );

      // clean data
      await m.delete();


      // testcase: seqDB-21817:limit/order/select/where组合查询

      // 准备数据
      docs = [
         { "_id": 1, "a": 1, "b": 3 },
         { "_id": 2, "a": 1, "b": 2 },
         { "_id": 3, "a": 2, "b": 1 },
         { "_id": 4, "a": 2, "b": 2 },
         { "_id": 5, "a": 3, "b": 3 },
         { "_id": 6, "a": 3, "b": 1 }
      ];
      await m.addMany( docs );

      // a.limit只指定offset，order指定为正序，find不指定options，where不带条件
      rc = await m.where( { "_id": { "$gt": 1 } } ).limit( 3 ).order( { "a": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":2,"a":1,"b":2},{"_id":3,"a":2,"b":1},{"_id":4,"a":2,"b":2}]' );
      // b.limit指定offset和length，length>offset，order指定为逆序，select指定options，where带条件匹配部分数据
      rc = await m.where( { "_id": { "$gt": 1 } } ).limit( 2, 6 ).order( { "a": -1 } ).select( { "t": "" } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":3,"a":2,"b":1},{"_id":4,"a":2,"b":2},{"_id":2,"a":1,"b":2}]' );
      // c.limit多条记录，order指定多字段正逆序
      rc = await m.where( { "_id": { "$gt": 1 } } ).limit( 3 ).order( { "a": -1, "b": 1 } ).select( { "t": "" } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":6,"a":3,"b":1},{"_id":5,"a":3,"b":3},{"_id":3,"a":2,"b":1}]' );
      // d.limit多条记录，order指定多字段逆正序
      rc = await m.where( { "_id": { "$gt": 1 } } ).limit( 3 ).order( { "a": 1, "b": -1 } ).select( { "t": "" } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":2,"a":1,"b":2},{"_id":4,"a":2,"b":2},{"_id":3,"a":2,"b":1}]' );
      // e.limit指定length=offset
      rc = await m.where().limit( 3, 3 ).order( { "_id": -1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":3,"a":2,"b":1},{"_id":2,"a":1,"b":2},{"_id":1,"a":1,"b":3}]' );
      // f.limit指定length<offset
      rc = await m.where().limit( 3, 2 ).order( { "_id": -1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":3,"a":2,"b":1},{"_id":2,"a":1,"b":2}]' );

      await m.delete();


      // testcase: seqDB-21822:countSelect/page查询

      // 准备数据
      docs = [
         { "_id": 1, "a": 1 },
         { "_id": 2, "a": 2 },
         { "_id": 3, "a": 3 },
         { "_id": 4, "a": 4 },
         { "_id": 5, "a": 5 },
         { "_id": 6, "a": 6 },
         { "_id": 7, "a": 7 }
      ];
      await m.addMany( docs );

      // model.countSelect( options )
      // a.不指定参数
      rc = await m.countSelect();
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":null,"currentPage":null,"data":[{"_id":1,"a":1},{"_id":2,"a":2},{"_id":3,"a":3},{"_id":4,"a":4},{"_id":5,"a":5},{"_id":6,"a":6},{"_id":7,"a":7}]}' );
      // b.只指定options
      rc = await m.countSelect( { "a": 1 } );
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":null,"currentPage":null,"data":[{"_id":1,"a":1},{"_id":2,"a":2},{"_id":3,"a":3},{"_id":4,"a":4},{"_id":5,"a":5},{"_id":6,"a":6},{"_id":7,"a":7}]}' );

      // m.page(page, pagesize).countSelect(options, pageFlag)
      // a.page接口page和pagesize均大于0，countSelect不指定pageFlage（默认）
      rc = await m.page( 3, 2 ).countSelect();
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":4,"pageSize":2,"currentPage":3,"data":[{"_id":5,"a":5},{"_id":6,"a":6}]}' );
      // b.page接口指定page和pagesize，其中page>总页数，countSelect指定options为空、pageFlag为true
      rc = await m.page( 5, 2 ).countSelect( {}, true );
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":4,"pageSize":2,"currentPage":1,"data":[{"_id":1,"a":1},{"_id":2,"a":2}]}' );
      // c.page接口指定page和pagesize，其中page>总页数，countSelect指定options和pageFlag，其中pageFlag为false
      rc = await m.page( 5, 2 ).countSelect( { "a": 1 }, false );
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":4,"pageSize":2,"currentPage":4,"data":[{"_id":7,"a":7}]}' );
      // d.不指定参数
      rc = await m.page().countSelect();
      Assert.equal( JSON.stringify( rc ), '{"count":7,"totalPages":1,"pageSize":10,"currentPage":1,"data":[{"_id":1,"a":1},{"_id":2,"a":2},{"_id":3,"a":3},{"_id":4,"a":4},{"_id":5,"a":5},{"_id":6,"a":6},{"_id":7,"a":7}]}' );

      await m.delete();


      // seqDB-21823:field显示字段

      // 准备数据
      docs = [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 2, "a": 2, "b": 1 }
      ];
      await m.addMany( docs );

      // model.field(field)
      // 只选择单个字段
      // a.选择_id
      rc = await m.field( "_id" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1},{"_id":2}]' );
      // b.选择普通字段，字段存在
      rc = await m.field( "a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":1},{"_id":2,"a":2}]' );
      // c.字段不存在
      rc = await m.field( "notExistField" ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1},{"_id":2}]' );
      // d.无参
      rc = await m.field().select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":1},{"_id":2,"a":2,"b":1}]' );

      // 选择多个字段
      // a.选择_id和普通字段
      rc = await m.field( "_id, a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":1},{"_id":2,"a":2}]' );
      // b.只选择普通字段
      rc = await m.field( "a, b" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":1,"b":1},{"_id":2,"a":2,"b":1}]' );
      // c.选择存在和不存在的字段
      rc = await m.field( "a, notExistField" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":1},{"_id":2,"a":2}]' );

      // 跟where组合
      // a.匹配到部分记录
      rc = await m.field( "a" ).where( { "a": 1 } ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":1}]' );
      // b.匹配到0条记录
      rc = await m.field( "a" ).where( { "notExistField": 1 } ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[]' );

      // 集合为空
      await m.delete();
      rc = await m.field( "a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[]' );

      await m.delete();


      // seqDB-21824:group分组

      // 准备数据
      docs = [
         { "_id": 1, "a": "Tom" },
         { "_id": 2, "a": "Lily" },
         { "_id": 3, "a": "Tom" }
      ];
      await m.addMany( docs );

      // model.group(group) 
      // 如下结果跟mongodb引擎结果均一致。m.group必须跟select一起用。
      // 字段存在
      rc = await m.group( "a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[{"_id":1,"a":"Tom"},{"_id":2,"a":"Lily"},{"_id":3,"a":"Tom"}]' );
      // 字段不存在
      rc = await m.group( "t" ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"Tom"},{"_id":2,"a":"Lily"},{"_id":3,"a":"Tom"}]' );
      // 无参
      rc = await m.group().select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"Tom"},{"_id":2,"a":"Lily"},{"_id":3,"a":"Tom"}]' );
      // null
      rc = await m.group( null ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs ) );
      // num
      rc = await m.group( 1 ).select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs ) );

      await m.delete();


      // testcase: seqDB-21825:distinct去重

      // 准备数据
      docs = [
         { "_id": 1, "a": "Tom" },
         { "_id": 2, "a": "Lily" },
         { "_id": 3, "a": "Tom" }
      ];
      await m.addMany( docs );

      // model.distinct(distinct)
      // 字段存在
      rc = await m.distinct( "a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '["Lily","Tom"]' );
      // 字段不存在
      rc = await m.distinct( "t" ).select();
      Assert.equal( JSON.stringify( rc ), '[]' );
      // 无参
      rc = await m.distinct().select();
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs ) );

      // 跟where组合使用
      // a.匹配到部分记录
      rc = await m.distinct( "a" ).where( { "_id": { "$in": [1, 3] } } ).select();
      Assert.equal( JSON.stringify( rc ), '["Tom"]' );
      // b.匹配到0条记录
      rc = await m.distinct( "a" ).where( { "b": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[]' );

      // 集合为空，去重
      await m.delete();
      rc = await m.distinct( "a" ).select();
      Assert.equal( JSON.stringify( rc.sort() ), '[]' );


      // testcase: seqDB-21826:sum求和

      // 准备数据
      docs = [
         { "_id": 1, "a": 1 },
         { "_id": 2, "a": 2 },
         { "_id": 3, "a": 3 }
      ];
      await m.addMany( docs );

      // model.sum(field)
      // 字段存在
      rc = await m.sum( "a" );
      Assert.equal( rc, 6 );
      // 字段不存在
      rc = await m.sum( "t" );
      Assert.equal( rc, 0 );
      // 无参
      rc = await m.sum();
      Assert.equal( rc, 0 );

      await m.delete();


      // testcase: seqDB-21827:aggregate聚集操作

      // model.aggregate(options) 
      // 准备数据
      docs = [
         { "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 },
         { "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 },
         { "_id": 3, "a": "C", "b": 100, "c": "test", "d": 2 },
         { "_id": 4, "a": "D", "b": 79, "c": "dev", "d": 2 }
      ];
      await m.addMany( docs );

      // $progect
      // 选择单个普通字段
      // a:1
      rc = await m.aggregate( { "$project": { "a": 1 } }, { "$sort": { "_id": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"A"},{"_id":2,"a":"B"},{"_id":3,"a":"C"},{"_id":4,"a":"D"}]' );
      // a:0
      try
      {
         await m.aggregate( { "$project": { "a": 0 } } );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -32 )
         {
            Assert.fail( "expect e.code = -32, actual e.code = " + e.code );
         }
      }

      // 选择单个_id字段
      // _id:1
      rc = await m.aggregate( { "$project": { "_id": 1 } }, { "$sort": { "_id": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1},{"_id":2},{"_id":3},{"_id":4}]' );
      // _id:0
      try
      {
         await m.aggregate( { "$project": { "_id": 0 } }, { "$sort": { "_id": 1 } } );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -32 )
         {
            Assert.fail( "expect e.code = -32, actual e.code = " + e.code );
         }
      }

      // 选择多个普通字段 
      // a:1,b:1
      rc = await m.aggregate( { "$project": { "a": 1, "b": 1 } }, { "$sort": { "_id": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"A","b":86},{"_id":2,"a":"B","b":90},{"_id":3,"a":"C","b":100},{"_id":4,"a":"D","b":79}]' );
      // a:1,b:0
      rc = await m.aggregate( { "$project": { "a": 1, "b": 0 } }, { "$sort": { "a": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"A"},{"_id":2,"a":"B"},{"_id":3,"a":"C"},{"_id":4,"a":"D"}]' );

      // 选择_id和普通字段
      // _id:1,b:1
      rc = await m.aggregate( { "$project": { "_id": 1, "b": 1 } }, { "$sort": { "_id": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"b":86},{"_id":2,"b":90},{"_id":3,"b":100},{"_id":4,"b":79}]' );
      // _id:0,b:1
      rc = await m.aggregate( { "$project": { "_id": 0, "b": 1 } }, { "$sort": { "b": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"b":79},{"b":86},{"b":90},{"b":100}]' );
      // _id:0,b:1,c:0
      rc = await m.aggregate( { "$project": { "_id": 0, "b": 1, "c": 0 } }, { "$sort": { "b": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"b":79},{"b":86},{"b":90},{"b":100}]' );
      // _id:0,b:1,c:1
      rc = await m.aggregate( { "$project": { "_id": 0, "b": 1, "c": 1 } }, { "$sort": { "b": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"b":79,"c":"dev"},{"b":86,"c":"test"},{"b":90,"c":"dev"},{"b":100,"c":"test"}]' );
      // _id:0,a:0
      try
      {
         await m.aggregate( { "$project": { "_id": 0, "a": 0 } }, { "$sort": { "_id": 1 } } );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -32 )
         {
            Assert.fail( "expect e.code = -6, actual e.code = " + e.code );
         }
      }

      // $group
      // $first / $last
      rc = await m.aggregate( [{ "$group": { "_id": "$c", "first_c": { "$first": "$c" }, "max_b": { "$max": "$b" }, "last_c": { "$last": "$c" } } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev","first_c":"dev","max_b":90,"last_c":"dev"},{"_id":"test","first_c":"test","max_b":100,"last_c":"test"}]' );
      // $max / $min
      rc = await m.aggregate( [{ "$group": { "_id": "$c", "max_b": { "$max": "$b" }, "min_b": { "$min": "$b" } } }, { "$sort": { "_id": 1 } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev","max_b":90,"min_b":79},{"_id":"test","max_b":100,"min_b":86}]' );
      // $push       
      rc = await m.aggregate( [{ "$group": { "_id": "$c", "push_d": { "$push": "$d" } } }, { "$sort": { "_id": 1 } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev","push_d":[1,2]},{"_id":"test","push_d":[1,2]}]' );
      // $avg, addToSet
      rc = await m.aggregate( [{ "$group": { "_id": "$c", "avg_b": { "$avg": "$b" }, "addtoset_d": { "$addToSet": "$d" } } }, { "$sort": { "_id": 1 } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev","avg_b":84.5,"addtoset_d":[1,2]},{"_id":"test","avg_b":93,"addtoset_d":[1,2]}]' );
      // $sum
      rc = await m.aggregate( [{ "$group": { "_id": "$c", "sum_b": { "$sum": "$b" } } }, {
         "$sort": { "_id": 1 }
      }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev","sum_b":169},{"_id":"test","sum_b":186}]' );
      // _id: null
      rc = await m.aggregate( [{ "$group": { "_id": "$c" } }, { "$sort": { "_id": 1 } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev"},{"_id":"test"}]' );
      // _id: num
      rc = await m.aggregate( [{ "$group": { "_id": "$c" } }, { "$sort": { "_id": 1 } }] );
      Assert.equal( JSON.stringify( rc ), '[{"_id":"dev"},{"_id":"test"}]' );

      // $match      
      // 匹配返回多条记录
      rc = await m.aggregate( { "$match": { "b": { "$gte": 90 } } }, { "$project": { "b": 1 } }, { "$sort": { "_id": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":2,"b":90},{"_id":3,"b":100}]' );
      // 返回0条记录
      rc = await m.aggregate( { "$match": { "b": { "$lt": 60 } } } );
      Assert.equal( JSON.stringify( rc ), '[]' );

      // $sort
      // 单个字段排序在其他测试点已覆盖
      // 多个字段正逆序
      rc = await m.aggregate( { "$match": { "b": { "$gte": 70 } } }, { "$sort": { "d": -1, "b": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":4,"a":"D","b":79,"c":"dev","d":2},{"_id":3,"a":"C","b":100,"c":"test","d":2},{"_id":1,"a":"A","b":86,"c":"test","d":1},{"_id":2,"a":"B","b":90,"c":"dev","d":1}]' );

      // $limit
      rc = await m.aggregate( { "$limit": 3 }, { "$sort": { "b": 1 } } );
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":"A","b":86,"c":"test","d":1},{"_id":2,"a":"B","b":90,"c":"dev","d":1},{"_id":3,"a":"C","b":100,"c":"test","d":2}]' );

      // 无效参数
      try
      {
         await m.aggregate( "" );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.message !== 'Invalid Argument' )
         {
            throw e;
         }
      }

      await m.delete();


      // seqDB-21820:匹配符测试

      // 准备数据
      docs = [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 2, "a": 2, "b": 1 },
         { "_id": 3, "a": 3, "b": 2 },
         { "_id": 4, "a": 4, "b": 2 }
      ];
      await m.addMany( docs );

      // $ne
      rc = await m.order( { "_id": 1 } ).where( { "b": { "$ne": 1 } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":3,"a":3,"b":2},{"_id":4,"a":4,"b":2}]' );

      // $and / $lt / $gt
      rc = await m.order( { "_id": 1 } ).where( { "$and": [{ "a": { "$gt": 1 } }, { "a": { "$lt": 4 } }] } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":2,"a":2,"b":1},{"_id":3,"a":3,"b":2}]' );

      // $or / $lte / $gte
      rc = await m.order( { "_id": 1 } ).where( { "$or": [{ "a": { "$lte": 1 } }, { "a": { "$gte": 4 } }] } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":1},{"_id":4,"a":4,"b":2}]' );

      // $exists
      await m.add( { "_id": 5, "a": 5, "c": 1 } );
      rc = await m.order( { "_id": 1 } ).where( { "c": { "$exists": 1 } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":5,"a":5,"c":1}]' );

      // $mod
      rc = await m.order( { "_id": 1 } ).where( { "$and": [{ "a": { "$lt": 5 } }, { "a": { "$mod": [3, 1] } }] } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":1},{"_id":4,"a":4,"b":2}]' );

      // $regex
      await m.addMany( [{ "_id": 6, "c": "abc" }, { "_id": 7, "c": "test" }] );
      rc = await m.order( { "_id": 1 } ).where( { "c": { "$regex": "^a", "$options": "i" } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":6,"c":"abc"}]' );

      // array
      // $all
      await m.addMany( [{ "_id": 8, "c": [1, 2] }, { "_id": 9, "c": [1, 2, 3] }] );
      rc = await m.order( { "_id": 1 } ).where( { "c": { "$all": [2, 3] } } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":9,"c":[1,2,3]}]' );


      // seqDB-21968:更新符测试（覆盖upsert）

      await m.delete();

      // 记录不存在, upsert:true
      rc = await m.where( { "a": 1 } ).update( { "$set": { "_id": 1, "a": 1 } }, { "upsert": true } );
      Assert.equal( rc, 0 );
      // 记录不存在, upsert:false
      rc = await m.where( { "a": 2 } ).update( { "$set": { "_id": 2, "a": 2 } }, { "upsert": false } );
      Assert.equal( rc, 0 );
      // 记录存在, upsert:false 
      rc = await m.where( { "a": 1 } ).update( { "$set": { "b": 1 } }, { "upsert": true } );
      Assert.equal( rc, 1 );
      // 检查结果
      rc = await m.select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":1}]' );

      await m.delete();

      // $setOnInsert, 匹配不存在的记录, upsert:ture|false
      // $setOnInsert + upsert:true
      rc = await m.where( { "a": 1 } ).update( { "$setOnInsert": { "_id": 1, "a": 1 } }, { "upsert": true } );
      Assert.equal( rc, 0 );
      // $setOnInsert + upsert:false
      rc = await m.where( { "a": 2 } ).update( { "$setOnInsert": { "_id": 2, "a": 2 } }, { "upsert": false } );
      Assert.equal( rc, 0 );
      // 检查结果 
      rc = await m.select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1}]' );

      // 记录不存在，$set + $setOnInsert组合
      // $set + $setOnInsert + upsert:true 
      rc = await m.where( { "a": 3 } ).update( { "$set": { "_id": 3 }, "$setOnInsert": { "a": 3 } }, { "upsert": true } );
      Assert.equal( rc, 0 );
      // $inc + $setOnInsert + upsert:true 
      rc = await m.where( { "a": 4 } ).update( { "$inc": { "a": 1 }, "$setOnInsert": { "_id": 4 } }, { "upsert": true } );
      Assert.equal( rc, 0 );
      // 检查结果 
      rc = await m.order( { "_id": 1 } ).select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1},{"_id":3,"a":3},{"_id":4,"a":5}]' );

      await m.delete();

      // 记录存在，$inc + $setOnInsert组合，upsert:true 
      await m.add( { "_id": 1, "a": 1 } );
      rc = await m.where( { "a": 1 } ).update( { "$set": { "b": 1 }, "$setOnInsert": { "c": 1 } }, { "upsert": true } );
      Assert.equal( rc, 1 );
      // 检查结果 
      rc = await m.select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1,"b":1}]' );

      // $setOnInsert，upsert:true, 字段不包含"_id" 
      rc = await m.where( { "a": 2 } ).update( { "$set": { "b": 2 }, "$setOnInsert": { "c": 2 } }, { "upsert": true } );
      Assert.equal( rc, 0 );
      // 检查结果 
      rc = await m.where( { "a": 2 } ).distinct( "a" ).select();
      Assert.equal( JSON.stringify( rc ), '[2]' );

      await m.delete();

      // others
      await m.addMany( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }] );
      // match multi docs, multi:true
      rc = await m.where( { "a": { "$gt": 1 } } ).update( { "$inc": { "b": 1 } }, { "multi": true } );
      Assert.equal( rc, 2 );
      // 检查结果
      rc = await m.select();
      Assert.equal( JSON.stringify( rc ), '[{"_id":1,"a":1},{"_id":2,"a":2,"b":1},{"_id":3,"a":3,"b":1}]' );

      // doc not exist, multi:false
      try
      {
         await m.where( { "a": { "$gt": 1 } } ).update( {}, { "multi": false } );
      }
      catch( e )
      {
         if( e.message !== "multi update is not supported for replacement-style update" )
         {
            Assert.fail( "e.message: " + e.message );
         }
      }

      await m.delete();


      // testcase: seqDB-21821:where查询操作符测试
      // TOD: 操作符设置不生效，貌似不支持，暂不关注
      // rc = await m.where( { }, { "b": { "$elemMatch": { "b1": 100 } } } ).select();        

      /*
            // model.parseOptions测试  ---fap暂不支持

            // 准备数据
            docs = [
               { "_id": 1, "a": 1, "b": "g1" },
               { "_id": 2, "a": 2, "b": "g2" },
               { "_id": 3, "a": 3, "b": "g1" }
            ];
            await m.addMany( docs );

            // $field
            rc = await m.find( { "field": "a" } );
            Assert.equal( JSON.stringify( rc ), '{"_id":1,"a":1}' );

            // $where
            rc = await m.find( { "where": { "a": 2 } } );
            Assert.equal( JSON.stringify( rc ), '{"_id":2,"a":2,"b":"g2"}' );

            // 其他

            await m.delete();
      */


      // seqDB-21994:增删改查大量数据

      // 准备数据
      docs = [];
      for( let i = 0; i < 2100; i++ )
      {
         docs.push( { "_id": i, "a": i, "b": 1 } );
      }
      await m.addMany( docs );

      // find
      // rc all records
      let expRecsNum = docs.length;
      rc = await m.order( { "_id": 1 } ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs ) );
      // rc recsNum = 1001
      expRecsNum = 1001;
      rc = await m.order( { "_id": 1 } ).limit( 0, 1001 ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs.slice( 0, expRecsNum ) ) );
      // rc recsNum = 1000
      expRecsNum = 1000;
      rc = await m.order( { "_id": 1 } ).limit( 0, 1000 ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs.slice( 0, expRecsNum ) ) );
      // rc recsNum = 999
      expRecsNum = 999;
      rc = await m.order( { "_id": 1 } ).limit( 0, 999 ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs.slice( 0, expRecsNum ) ) );

      // update      
      expDocs = [];
      expRecsNum = 2010;
      for( let i = 0; i < expRecsNum; i++ )
      {
         expDocs.push( { "_id": i, "a": i, "b": 2 } );
      }
      rc = await m.where( { "a": { "$lt": expRecsNum } } ).update( { "$inc": { "b": 1 } } );
      Assert.equal( rc, expRecsNum );
      // check result for update
      rc = await m.where( { "a": { "$lt": expRecsNum } } ).order( { "_id": 1 } ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( expDocs ) );
      // check result for not update      
      rc = await m.where( { "a": { "$gte": expRecsNum } } ).order( { "_id": 1 } ).select();
      Assert.equal( rc.length, ( docs.length - expRecsNum ) );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( docs.slice( expRecsNum, docs.length ) ) );

      // delete
      expRecsNum = 10;
      let deleteRC21994 = await m.where( { "a": { "$gte": expRecsNum } } ).delete();
      Assert.equal( deleteRC21994, docs.length - expRecsNum );
      // check result
      rc = await m.order( { "_id": 1 } ).select();
      Assert.equal( rc.length, expRecsNum );
      Assert.equal( JSON.stringify( rc ), JSON.stringify( expDocs.slice( 0, expRecsNum ) ) );

      // clean data
      await m.delete();


      //testcase: seqDB-21829:创建/删除/列取索引/添加唯一键重复数据

      // ready data      
      docs = [
         { "_id": 1, "a": 1, "b": 1, "c": 1 },
         { "_id": 2, "a": 2, "b": 2, "c": 1 },
         { "_id": 3, "a": 3, "b": 3, "c": 3 }
      ];
      await m.addMany( docs );

      // model.createIndex(indexes,options)
      // normal index
      rc = await m.createIndex( { "a": 1 } );
      Assert.equal( rc, 'a_1' );
      // unique index
      rc = await m.createIndex( { "b": 1 }, { "unique": true } );
      Assert.equal( rc, 'b_1' );
      // getIndexes
      rc = await m.getIndexes();
      Assert.equal( JSON.stringify( rc ),
         '[{"v":0,"key":{"_id":1},"name":"_id_","ns":"' + clName + '"},{"v":0,"key":{"a":1},"name":"a_1","ns":"' + clName + '"},{"v":0,"unique":true,"key":{"b":1},"name":"b_1","ns":"' + clName + '"}]' );

      // duplicate index
      // the same key and define
      rc = await m.createIndex( { "a": 1 } );
      Assert.equal( rc, 'a_1' );
      // the same key, bug different define
      rc = await m.createIndex( { "b": 1 }, { "unique": false } );
      Assert.equal( rc, 'b_1' );

      // exist duplicate key, create unique index    
      try
      {
         await m.createIndex( { "c": 1 }, { "unique": true } );
         //TODO SEQUOIADBMAINSTREAM-5513
         //Assert.fail( "expect fail, but actual success" ); 
      }
      catch( e )
      {
         if( e.code !== 'Duplicate key exist' )
         {
            Assert.fail( "expect e.code = -6, actual e.code = " + e.code );
         }
      }

      // exist unique index, add duplicate key
      try
      {
         await m.add( { "_id": 4, "b": 1 } );
         Assert.fail( "expect fail, but actual success" );
      }
      catch( e )
      {
         if( e.code !== -38 )
         {
            Assert.fail( "expect e.code = -6, actual e.code = " + e.code );
         }
      }

      // check result
      let selectForidxRC = await m.order( { "_id": 1 } ).select();
      Assert.equal( JSON.stringify( selectForidxRC ), '[{"_id":1,"a":1,"b":1,"c":1},{"_id":2,"a":2,"b":2,"c":1},{"_id":3,"a":3,"b":3,"c":3}]' );

      // clean data
      await m.delete();


      console.log( "test success, end." );
      this.assign( "m", await m.select() );
      return this.display();
   }

};
