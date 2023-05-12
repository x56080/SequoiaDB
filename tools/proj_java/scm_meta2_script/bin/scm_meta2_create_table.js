/**
 * Js file for creating tables in test cluster for fixing scm meta data.
 * 
 * Usage:
 * ./sdb -e ' var HOST = "192.168.17.37:11810"; var USER = "abc"; var PASSWD = "abc"; var METATABLE = "IBSFLOW.FILE_2021"; var FORCE = false;' -f scm_meta2_create_table.js
 *
 * Changes:
 * 2023/5/8 第一版本，初始化
 */

const DOMAIN_DEFAULT          = "domain3";     // 可以根据测试环境的情况调整域的名字
const TYPE_UNDEF              = "undefined";
const SUFFIX_CS_NAME          = "_test";
const SUFFIX_META_CL          = "_meta";
const SUFFIX_LOB_INFO_CL      = "_lobinfo";
const SUFFIX_META_UPDATE_CL   = "_update";
const IDX_USER_DEF_OID        = "idx_oid";     // 不可以改动。后续其它地方需使用该名字的索引

function main() {
   // init
   if ( typeof(HOST) == TYPE_UNDEF ||
      typeof(USER) == TYPE_UNDEF ||
      typeof(PASSWD) == TYPE_UNDEF ||
      typeof(METATABLE) == TYPE_UNDEF ) {
      throw new Error("Invalid HOST/USER/PASSWD/METACL arguments") ;
   }

   var forceCreate = false;
   if (typeof(FORCE) != TYPE_UNDEF && FORCE == true) {
      forceCreate = true;
   }

   var hostName = HOST.split(":")[0];
   var port = HOST.split(":")[1];
   
   var csName = METATABLE.split(".")[0];
   var clName = METATABLE.split(".")[1];

   var newCSName = csName + SUFFIX_CS_NAME;
   var metaCLName = clName + SUFFIX_META_CL;
   var lobInfoCLName = clName + SUFFIX_LOB_INFO_CL;
   var metaUpdateCLName = clName + SUFFIX_META_UPDATE_CL;
   
   var sdb = new Sdb( hostName, port, USER, PASSWD ) ;
   try {
      // test domain exists or not
      try {
         sdb.getDomain(DOMAIN_DEFAULT);
      } catch( e ) {
         throw new Error("Failed to get domain: " + DOMAIN_DEFAULT + ", please check. Error: " + e);
      }

      // try to drop cs before creating
      if (forceCreate) {
         try {
            sdb.dropCS(newCSName);
            println("Drop cs successfully!");
            sleep(2000);
         } catch( e ) {
            // ignore
         }
      }

      // create tables
      var cs = sdb.createCS(newCSName, {Domain: DOMAIN_DEFAULT});
      cs.createCL(metaCLName, {ShardingKey:{"_id":1}, AutoSplit: true});
      cs.createCL(lobInfoCLName, {ShardingKey:{"_id":1}, AutoSplit: true});
      cs.createCL(metaUpdateCLName); // 这个表的记录数应该很小，没必要切分
      sleep(2000); // sleep 2 secs

      // create index
      cs.getCL(lobInfoCLName).createIndex(IDX_USER_DEF_OID, {"Oid": 1});
   } finally {
      sdb.close() ;
   }

   println("Success to create tables!");
}

main () ;