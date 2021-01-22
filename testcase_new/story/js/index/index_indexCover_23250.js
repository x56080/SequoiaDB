/******************************************************************************
 * @Description   : seqDB-23250:索引不支持数组，非嵌套对象+复合索引，选择条件索引字段不包含排序索引字段
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.01.09
 * @LastEditors   : Xiaoni Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23250";
testConf.clOpt = { "ReplSize": -1 };

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 创建索引   
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1, "c": 1, "d": 1 }, { "NotArray": true } )

   // 插入数据，recordsNum不能小于1000（查询条件限制）
   var recordsNum = 2000;
   var records = new Array();
   for( var i = 0; i < recordsNum; i++ )
   {
      records.push( { "a": i, "b": i, "c": ( ( recordsNum - 1 ) - i ), "d": i } );
   }
   cl.insert( records );

   // 1）不等于（如：选择字段为a，排序字段为b）
   try
   {
      // 开启覆盖索引开关
      db.updateConf( { "indexcoveron": true } );

      // 检查索引访问回表
      var cond = { "a": { "$gt": 100 }, "c": { "$lt": 1000 } };
      var sel = { "a": "", "b": "" };
      var sortCond = { "c": 1, "d": 1 };
      var hint = { "": idxName };
      db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + testConf.clName } );
      var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
      var expIndexCover = false;
      // 独立模式/直连数据索引访问不回表，见SEQUOIADBMAINSTREAM-6652，此处根据独立模式做区分，保证独立模式下查询数据没有问题
      if( commIsStandalone( db ) )
      {
         expIndexCover = true;
      }
      assert.equal( JSON.parse( explainInfo[0] ).IndexCover, expIndexCover, "explainInfo = " + explainInfo );

      // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
      var obj1 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

      db.updateConf( { "indexcoveron": false } );
      var obj2 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

      commCompareObject( obj1, obj2 );
   }
   finally
   {
      db.updateConf( { "indexcoveron": true } );
   }


   // 2）小于（如：选择字段为a，排序字段为a+b））
   try
   {
      // 开启覆盖索引开关
      db.updateConf( { "indexcoveron": true } );

      // 检查索引访问回表
      var cond = { "a": { "$gt": 100 }, "b": { "$field": "a" } };
      var sel = { "a": "", "b": "" };
      var sortCond = { "a": 1, "b": 1, "c": 1 };
      var hint = { "": idxName };
      db.analyze( { "Mode": 5, "Collection": COMMCSNAME + "." + testConf.clName } );
      var explainInfo = cl.find( cond, sel ).sort( sortCond ).hint( hint ).explain().toArray();
      // 独立模式/直连数据索引访问不回表，见SEQUOIADBMAINSTREAM-6652，此处根据独立模式做区分，保证独立模式下查询数据没有问题
      var expIndexCover = false;
      if( commIsStandalone( db ) )
      {
         expIndexCover = true;
      }
      assert.equal( JSON.parse( explainInfo[0] ).IndexCover, expIndexCover, "explainInfo = " + explainInfo );

      // 检查查询数据正确性（开启覆盖索引和不开启覆盖索引相同查询语句结果做对比）
      var obj1 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

      db.updateConf( { "indexcoveron": false } );
      var obj2 = cl.find( cond, sel ).sort( sortCond ).hint( hint ).toArray();

      commCompareObject( obj1, obj2 );
   }
   finally
   {
      db.updateConf( { "indexcoveron": true } );
   }
}