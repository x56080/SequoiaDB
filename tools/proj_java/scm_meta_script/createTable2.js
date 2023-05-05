/**
 * 提前创建一个叫 domain3 的域, 让其包含 三个数据组
 * 
 * /opt/sequoiadb/bin/sdb -e ' var ADDR="locahost:11810"; var CSNAME = "IBSFLOW"; ' -f createTable2.js
 */


if ( typeof (ADDR) == "undefined" || typeof (CSNAME) == "undefined" ) {
   throw Error("Error: invalid argument");
}

import("func.js")

var addr = ADDR;

var domainName = "domain3";
//var csName = "IBSFLOW2";
var csName = CSNAME;
var mainCLName = "FILE";
var subCL2020 = "FILE_2020";
var subCL2021 = "FILE_2021";
var subCL2022 = "FILE_2022";
var subCL2023 = "FILE_2023";

db = new Sdb(addr);

// create cs and cl
dropCS(db, csName);
sleep(2000);
db.createCS(csName, {Domain: domainName});
sleep(2000);
db.getCS(csName).createCL(mainCLName, {IsMainCL: true, ShardingKey:{"create_month":1}, ShardingType:"range"});
db.getCS(csName).createCL(subCL2020, {AutoSplit:true, ShardingKey:{_id:1}});
sleep(2000);

// attach cl
db.getCS(csName).getCL(mainCLName).attachCL(csName + "." + subCL2020, {LowBound:{"create_month": "2020"}, UpBound:{"create_month": "2021"}});
