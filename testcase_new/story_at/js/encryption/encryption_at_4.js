/***************************************************************************************************
 * @Description: 集合同时开启压缩与加密
 * @ATCaseID: encryption_4
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 03/22/2023 Zhou Hongye    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    集合同时开启压缩与加密，为确保生成压缩字典，需要插入足够的数据，验证记录数据是否符合预期
 * 测试步骤：
 *    1.创建普通集合，开启压缩与加密
 *    2.插入足够的数据确保生成了压缩字典
 *    3.读取数据，校验数据正确性
 * 期望结果：
 *    记录数据符合预期
 **************************************************************************************************/
testConf.clName = "encryption_4";
testConf.clOpt = { Encrypted: true, Compressed: true };
main(test);
function test(testPara) {
  var cl = testPara.testCL;
  var text = "";
  for (var i = 0; i < 1024; i++) {
    text += "t";
  }

  var maxTimes = 10;
  var currentTimes = 0;
  var records = [];
  while (!hasCreatedDictionary(cl)) {
    if (currentTimes > maxTimes) {
      throw Error(
        "Reached maximum times of inserting data before created dictionary"
      );
    }
    var arr = [];
    for (var i = 0; i < 10 * 1024; i++) {
      var r = { id: i + currentTimes * 10 * 1024, pad: text };
      arr.push(r);
      records.push(r);
    }
    cl.insert(arr);
    currentTimes++;
  }
  var cursor = cl.find();
  commCompareResults(cursor, records);
}

function hasCreatedDictionary(cl) {
  return cl.getDetail().current().toObj().Details[0].DictionaryCreated === true;
}
