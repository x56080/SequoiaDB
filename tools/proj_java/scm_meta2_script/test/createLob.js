
/**
 * /opt/sequoiadb/bin/sdb -e ' var FILE_NAME = "IBSFLOW.FILE_2020.meta.json"; var LOB_FILE = "a.txt"; ' -f createLob.js 
 */ 

var db1 = new Sdb ("192.168.30.35", 11810);
var db2 = new Sdb ("192.168.30.45", 11810);
var db3 = new Sdb ("192.168.30.46", 11810);

var mainSiteCL202011 = null;
var mainSiteCL202012 = null;
var subSite1CL = null;
var subSite2CL = null;

function dropCS(db, csName) {
   try {
      db.dropCS(csName);
   } catch( e ) {
      // ignore
   }
}

function insertMainSite(oid, createTime) {
   // 如果时间小于 2020-11-31 23.59.59, 插入 202011 表，否则插入  202012 表
   if (createTime < 1606838399000 ) {
      mainSiteCL202011.putLob(LOB_FILE, oid);
   } else {
      mainSiteCL202012.putLob(LOB_FILE, oid);
   }
}

function insertSubSite1(oid) {
   subSite1CL.putLob(LOB_FILE, oid);
}

function insertSubSite2(oid) {
   subSite2CL.putLob(LOB_FILE, oid);
}

function main() {
   /*
   lobid: xxxxx

   main: site_id:1
   IBSFLOW_202011.FILE_202011
   IBSFLOW_202012.FILE_202012

   sub: site_id:2
   IBSFLOW_2020.FILE_2020
   */

   // 先删除表
   dropCS(db1, "IBSFLOW_LOB_202011");
   dropCS(db1, "IBSFLOW_LOB_202012");
   dropCS(db2, "IBSFLOW_LOB_2020");
   dropCS(db3, "IBSFLOW_LOB_2020");
   sleep (2000);

   // 然后重新创建
   mainSiteCL202011 = db1.createCS("IBSFLOW_LOB_202011").createCL("LOB_202011");
   mainSiteCL202012 = db1.createCS("IBSFLOW_LOB_202012").createCL("LOB_202012");
   subSite1CL = db2.createCS("IBSFLOW_LOB_2020").createCL("LOB_2020");
   subSite2CL = db3.createCS("IBSFLOW_LOB_2020").createCL("LOB_2020");
   sleep (2000);

   var iterationCounter = 0;
   var file = new File(FILE_NAME);
   var line = null;

   while (true) {
      try {
         line = file.readLine();
      } catch(e) {
         if (-9 == e) {
            return; // 结束
         } else {
            throw e;
         }
      }
      if (line == "") {
         continue;
      }
      var obj = eval( '(' + line + ')' );
      var oid = obj["data_id"]; // lob oid
      var createTime = obj["data_create_time"];

      var count = iterationCounter++ % 10;
      if (count < 5) { // 50% 插入三个站点
         insertMainSite(oid, createTime);
         insertSubSite1(oid);
         insertSubSite2(oid);
      } else if (count >= 5 && count <= 7) { // 30% 插入两个站点
         var count2 = iterationCounter % 3;
         if (count2 == 0) {
            insertMainSite(oid, createTime);
            insertSubSite1(oid);
         } else if (count2 == 1) {
            insertMainSite(oid, createTime);
            insertSubSite2(oid);
         } else {
            insertSubSite1(oid);
            insertSubSite2(oid);
         }
      } else { // 20% 插入一个站点
         var count3 = iterationCounter % 3;
         if (count3 == 0) {
            insertMainSite(oid, createTime);
         } else if (count3 == 1) {
            insertSubSite1(oid);
         } else {
            insertSubSite2(oid);
         }
      }
   } // while

}

main();