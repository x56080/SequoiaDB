/******************************************************************************
 * @Description   : seqDB-32271:索引不支持数组，selector带scala函数
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.28
 * @LastEditTime  : 2023.06.28
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_indexCover_32271";

main( test );
function test ( testPara ) 
{
   try
   {
      db.updateConf( { "indexcoveron": true } );
      var cl = testPara.testCL;

      // 插入数据
      var records = [];
      for( var i = 0; i < 10; i++ )
      {
         records.push( { a: i, b: "string" + i, c: i } );
      }
      cl.insert( records )

      // 创建索引
      var indexName = "index_32271";
      cl.createIndex( indexName, { a: 1, b: 1 }, { NotArray: true } );

      // 收集统计信息
      db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );

      // 1.a是scalar函数
      var cursor = cl.find( { a: 1 }, { "a": { "$add": 1 } } );
      commCompareResults( cursor, [{ a: 2, b: "string" + 1 }] );
      var explain = cl.find( { a: 1 }, { "a": { "$add": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 2.a是{"a":{"$include":1}
      var cursor = cl.find( { a: 2 }, { "a": { "$include": 1 } } );
      commCompareResults( cursor, [{ a: 2 }] );
      var explain = cl.find( { a: 2 }, { "a": { "$include": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 3.a是{"a":{"$include":0}
      var cursor = cl.find( { c: { $gte: 8 } }, { "a": { "$include": 0 } } );
      commCompareResults( cursor, [{ b: "string" + 8, c: 8 }, { b: "string" + 9, c: 9 }] );
      var explain = cl.find( {}, { "a": { "$include": 0 } } ).explain();
      checkIndexCover( explain, false );

      // 4.a和b都是scalar函数
      var cursor = cl.find( { a: 0 }, { "a": { "$floor": 1 }, "b": { "$size": 1 } } );
      commCompareResults( cursor, [{ a: 0, b: null }] );
      var explain = cl.find( { a: 0 }, { "a": { "$floor": 1 }, "b": { "$size": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 5.非索引字段c是scalar函数
      var cursor = cl.find( { a: 1 }, { "c": { "$substr": 1 } } );
      commCompareResults( cursor, [{ a: 1, b: "string" + 1, c: null }] );
      var explain = cl.find( { a: 1 }, { "c": { "$substr": 1 } } ).explain();
      checkIndexCover( explain, false );

      // 6.非索引字段c是{"c":{"$include":1}
      var cursor = cl.find( { c: { $gte: 9 } }, { "c": { "$include": 1 } } );
      commCompareResults( cursor, [{ c: 9 }] );
      var explain = cl.find( {}, { "c": { "$include": 1 } } ).explain();
      checkIndexCover( explain, false );

      // 7.非索引字段c是{"c":{"$include":0}
      var cursor = cl.find( { b: "string1" }, { "c": { "$include": 0 } } );
      commCompareResults( cursor, [{ a: 1, b: "string" + 1 }] );
      var explain = cl.find( { b: "string1" }, { "c": { "$include": 0 } } ).explain();
      checkIndexCover( explain, false );

      // 8.a是scalar函数且$include为0，b是scalar函数
      var cursor = cl.find( { c: 5 }, { "a": { "$multiply": 1, "$include": 0 }, "b": { "$divide": 1 } } );
      commCompareResults( cursor, [{ b: null, c: 5 }] );
      var explain = cl.find( { c: 5 }, { "a": { "$multiply": 1, "$include": 0 }, "b": { "$divide": 1 } } ).explain();
      checkIndexCover( explain, false );

      // 9.a是scalar函数且$include为1，b是scalar函数
      var cursor = cl.find( { a: 9 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } );
      commCompareResults( cursor, [{ a: 9, b: 7 }] );
      var explain = cl.find( { a: 9 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 10.a是scalar函数，b是普通字段
      var cursor = cl.find( { a: 6 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } );
      commCompareResults( cursor, [{ a: 6, b: 7 }] );
      var explain = cl.find( { a: 6 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 11.a是{a:{$include:1}}，b是普通字段
      var cursor = cl.find( { a: 7 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } );
      commCompareResults( cursor, [{ a: 7, b: 7 }] );
      var explain = cl.find( { a: 7 }, { "a": { "$abs": 1, "$include": 1 }, "b": { "$strlen": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 12.a是普通字段，b是scalar函数
      var cursor = cl.find( { a: 8 }, { "a": 1, "b": { "$abs": 1 } } );
      commCompareResults( cursor, [{ a: 8, b: null }] );
      var explain = cl.find( { a: 8 }, { "a": 1, "b": { "$abs": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 13.a是scalar函数，b是scalar函数且$include为0
      var cursor = cl.find( { a: { "$lt": 1 } }, { "a": { "$multiply": 1 }, "b": { "$divide": 1, "$include": 0 } } );
      commCompareResults( cursor, [{ a: 0, c: 0 }] );
      var explain = cl.find( { a: { $lt: 1 } }, { "a": { "$multiply": 1 }, "b": { "$divide": 1, "$include": 0 } } ).explain();
      checkIndexCover( explain, false );

      // 14.a是scalar函数，b是scalar函数且$include为1
      var cursor = cl.find( { a: { "$et": 4 } }, { "a": { "$abs": 1 }, "b": { "$strlen": 1, "$include": 1 } } );
      commCompareResults( cursor, [{ a: 4, b: 7 }] );
      var explain = cl.find( { a: { "$et": 4 } }, { "a": { "$abs": 1 }, "b": { "$strlen": 1, "$include": 1 } } ).explain();
      checkIndexCover( explain, true );

      // 15.a和b都是scalar函数，c是普通字段
      var cursor = cl.find( { a: 3 }, { "a": { "$upper": 1 }, "b": { "$lower": 1 }, "c": 1 } );
      commCompareResults( cursor, [{ a: null, b: "string" + 3, c: 3 }] );
      var explain = cl.find( { a: 3 }, { "a": { "$upper": 1 }, "b": { "$lower": 1 }, "c": 1 } ).explain();
      checkIndexCover( explain, false );

      // 16.a和b都是scalar函数，c是{"c":{"$include":0}
      var cursor = cl.find( { a: 2 }, { "a": { "$concat": 1 }, "b": { "$default": 1 }, "c": { "$include": 0 } } );
      commCompareResults( cursor, [{ a: 2 + "1", b: "string" + 2 }] );
      var explain = cl.find( { a: 2 }, { "a": { "$concat": 1 }, "b": { "$default": 1 }, "c": { "$include": 0 } } ).explain();
      checkIndexCover( explain, false );

      // 17.a是普通字段，b是scalar函数且$include为0，c是普通字段
      var cursor = cl.find( { a: 6 }, { "a": 1, "b": { "$abs": 1, "$include": 0 }, "c": 1 } );
      commCompareResults( cursor, [{ a: 6, c: 6 }] );
      var explain = cl.find( { a: 6 }, { "a": 1, "b": { "$abs": 1, "$include": 0 }, "c": 1 } ).explain();
      checkIndexCover( explain, false );
   } finally
   {
      db.updateConf( { "indexcoveron": false } );
   }
}


function checkIndexCover ( explain, expResult )
{
   while( explain.next() )
   {
      var result = explain.current().toObj();
      var actResult = result.IndexCover;
      assert.equal( expResult, actResult, "explain = " + explain.toArray() );
   }
}