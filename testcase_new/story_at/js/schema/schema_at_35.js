/***************************************************************************************************
 * @Description: 当schema extent最大扩容到4MB时，能够得到预期错误
 * @ATCaseID: schema_34
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 03/02/2023 Zhou Hongye Create indexes on main-collection bound to schema which defines column read default
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    创建集合并绑定外部模式，为外部模式不断添加字段，直到达到schema extent存储的上限
 *
 * 测试步骤：
 *    1.创建集合并绑定外部模式
 *    2.为外部模式添加字段名大于127的字段，得到预期报错
 *    3.将已有字段重命名为长度大于127的名字，得到预期报错
 *    2.为外部模式不断添加字段，达到schema extent存储的上限时得到预期报错
 *
 * 期望结果：
 *    得到预期报错
 *
 **************************************************************************************************/
testConf.clName = "schema_35";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test(testPara) {
  var clName = testConf.clName;
  var cl = testPara.testCL;
  var schemaName = clName + "_schema";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, { a: {Type: "int32", ReadDefault: 5, WriteDefault: 10} });
  cl.addSchema(schemaName);
  var columnName = "";
  for( var i = 0;i < 130; i++) { columnName += "a";}
  assert.tryThrow(SDB_INVALIDARG, function () {
    schema.addColumn(columnName, {
      Type: "int32",
      ReadDefault: 5,
      WriteDefault: 10,
    });
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    schema.renameColumn("a", columnName);
  });

  var text = "default";
  for( var i = 0;i < 30000; i++) { text += "a";}

  var columnID = 0;
  while( true )
  {
    try{
      schema.addColumn("column" + columnID, { Type:"string", ReadDefault: text, WriteDefault: text} )
    }
    catch(e)
    {
      var err = e.message || e;
      if ( SDB_OSS_UP_TO_LIMIT == err )
      {
        break;
      }
      else
      {
        throw e;
      }
    }
    columnID++;
  }
}
