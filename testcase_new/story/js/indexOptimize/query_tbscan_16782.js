/************************************
*@Description: 查询字段包含部分索引字段，不匹配索引  
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16782
**************************************/
main();
function main()
{
   //create CL
   var clName = COMMCLNAME + "_tbscan_16782";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string"], ['a','b','c','d'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abcd", {a:1, b:1, c:1, d:1});
 
   var findCond = {b: {$gt: 1000}, c: {$gt: ""}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "tbscan");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}
