import("../lib/basic_operation/commlib.js");
import("../lib/main.js");


var tmpFileDir = WORKDIR + "/sdbimprt/";

var installDir = getInstallDir()

readyTmpDir()
/* ****************************************************
@description: ready tmp director
**************************************************** */
function readyTmpDir() {

   cmd.run("rm -rf " + tmpFileDir);

   cmd.run("mkdir -p " + tmpFileDir);
}

function importData(csName, clName, importFile, type, mode, matchfield, hint) {
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --file ' + importFile
      + ' --mode ' + mode
      + ' --matchfields ' + matchfield
      + ' --hint ' + hint
   var rc = cmd.run(imprtOption);
   var rcResults = rc.split("\n");

   return rcResults;
}

function exportData(csName, clName, exportFile, type) {
   var exprtOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --file ' + exportFile
   cmd.run(exprtOption);
}

function checkImportRC(rcResults, expUpdatedNum, expModifiedNum, expInsertedNum) {

   var expUpdatedRecords = "Updated records: " + expUpdatedNum;
   var expModifiedRecords = "Modified records: " + expModifiedNum;
   var expInsertedRecords = "Inserted records: " + expInsertedNum;
   var actUpdatedRecords = rcResults[4];
   var actModifiedRecords = rcResults[5];
   var actInsertedRecords = rcResults[6];
   if (expUpdatedRecords !== actUpdatedRecords
      || expModifiedRecords !== actModifiedRecords
      || expInsertedRecords !== actInsertedRecords) {
      throw new Error("importData fail,[sdbimprt results]" +
         "[" + expUpdatedRecords + ", " + expModifiedRecords + ", " + expInsertedRecords + "]" +
         "[" + actUpdatedRecords + ", " + actModifiedRecords + ", " + actInsertedRecords + "]");
   }
}

function checkCLData(cl, expResult) {
   var actResult = [];
   var cursor = cl.find({}, { "_id": { "$include": 0 } }).sort({ "_id": 1 });
   while (cursor.next()) {
      actResult.push(cursor.current().toObj());
   }
   for (var i in expResult) {
      if (JSON.stringify(expResult[i]) !== JSON.stringify(actResult[i])) {
         throw new Error("actResult: " + JSON.stringify(actResult[i]) + ", expResult: " + JSON.stringify(expResult[i]));
      }
   }
}

/* ****************************************************
@description: get install_dir of sequoiadb
@return: install_dir
**************************************************** */
function getInstallDir() {
   var localPath = cmd.run("pwd").split("\n")[0] + "/";
   var installPath = '';

   // 先取当前目录下的 bin/sdbimprt，不存在时，再取安装目录下的 bin/sdbimprt
   try {
      cmd.run('find ./bin/sdbimprt').split('\n')[0];
      installPath = localPath;
   }
   catch (e) {
      installPath = commGetInstallPath() + "/";
   }

   return installPath;
}

/* ****************************************************
@description: new File
@return: file
**************************************************** */
function fileInit(fileName) {
   var file = new File(fileName);
   return file;
}