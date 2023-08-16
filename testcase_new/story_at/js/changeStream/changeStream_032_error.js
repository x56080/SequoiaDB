/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_032
 * @Author: Huang Youquan
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 08/10/2023 Huang Youquan change stream
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    在节点异常时，协调节点自动订阅其他节点watch change stream
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "changeStream_032";
var selectedGroup = selectGroupWithSecondary(db);
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 };
main(test);
function test() {
   var node;
   var cur;
   try {
      var nodeCL = db.getCS(testConf.csName).getCL(testConf.clName);
      db.setSessionAttr({ PreferredInstance: [1, "S"] });
      cur = nodeCL.watch(StreamToken(), { MaxWaitTime: 0, Groups: [selectedGroup] });

      var document = { _id: 1, a: 1, b: 1 };
      var dbCL = db.getCS(testConf.csName).getCL(testConf.clName);

      // insert
      dbCL.insert(document);
      checkChangeStreamInsertResult(cur, testConf.csName, testConf.clName, document);
      node = getWatchNode(testConf.csName, testConf.clName, selectedGroup);
      node.stop();

      var documentKey = { _id: 1 };
      var updateAction = { $set: { a: 2 } };
      dbCL.update(updateAction, documentKey);
      checkChangeStreamUpdateResult(cur, testConf.csName, testConf.clName, documentKey, updateAction);

   }
   catch (e) {
      throw e;
   }
   finally {
      node.start();
      commCheckBusinessStatus(db);
      cur.close();
   }
}
