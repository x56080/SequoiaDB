/***************************************************************************************************
 * @Description: $ifnull功能测试
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/17/2023 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    验证$ifnull基本功能
 * 测试步骤：
 *    1. $ifnull使用fieldName，发起查询
 *    2. $ifnull使用fieldName1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，对于为空或不在的字段，返回指定值
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_ifnull_1";

main(test);
function test(args) {
  var cl = args.testCL;
  var values = [-2147483648, 2147483647, { "$numberLong": "-9223372036854775808" }, { "$numberLong": "9223372036854775807" },
  -1.7E+308, 1.7e+308, "", "test_id", { a: 1 }, true, { "$date": "0000-01-01" }, { "$date": "9999-12-31" },
  { "$timestamp": "1902-01-01-00.00.00.000000" }, { "$timestamp": "2037-12-31-23.59.59.999999" },
  { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" }, ["-2147483648", 0, "def"], [], null, { "$minKey": 1 }, { "$maxKey": 1 }, { "$regex": "^W", "$options": "i" }];

  insertRecords(cl, values);

  findRecords(cl, values);

  findRecords1(cl, values);
}

function insertRecords(cl, values) {
  var docs = [];
  for (var i = 0; i < values.length; ++i) {
    var fieldValue = values[i];
    var objs = { "no": i, "fieldName": fieldValue };
    docs.push(objs);
  }
  cl.insert(docs);
  return docs;
}

function findRecords(cl, values) {
  for (var i = 0; i < values.length; ++i) {
    var actRecords = cl.find({}, { "fieldName": { "$ifnull": values[i] } });
    var expRecords = expectRecords(values, i);
    commCompareResults(actRecords, expRecords);
  }
}

function expectRecords(values, num) {
  var docs = [];
  for (var i = 0; i < values.length; ++i) {
    var fieldValue = values[i];
    if (fieldValue === null) {
      var objs = { "no": i, "fieldName": values[num] };
      docs.push(objs);
    }
    else {
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push(objs);
    }
  }
  return docs;
}

function findRecords1(cl, values) {
  for (var i = 0; i < values.length; ++i) {
    var actRecords = cl.find({}, { "fieldName1": { "$ifnull": values[i] } });
    var expRecords = expectRecords1(values, i);
    commCompareResults(actRecords, expRecords);
  }
}

function expectRecords1(values, num) {
  var docs = [];
  for (var i = 0; i < values.length; ++i) {
    var objs = { "no": i, "fieldName": values[i], "fieldName1": values[num] };
    docs.push(objs);
  }
  return docs;
}