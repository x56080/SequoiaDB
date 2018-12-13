/************************************
*@Description: 多个索引与查询条件匹配字段相等，且索引定义字段也相等    
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16794
**************************************/
main();
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16794";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string", "string", "string", "string"], ['a','b','c','d','f','g','h'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abcd", {a:-1, b:1, c:-1, d:-1, h:1});
   commCreateIndex(dbcl, "abcfh", {b:-1, a:1, c:-1, f:-1, h:1});
   commCreateIndex(dbcl, "abcgh", {a:1, b:1, c:-1, g:1, h:-1});

   // query rule equal, index rule equal, choose the first index 
   var findCond = {a: {$gt: 10000}, b: {$lt: 10000}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "ixscan", "abcd");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}
