/************************************
*@Description: seqDB-16790 multiple indexes match fields for query conditions,
               the sort field matches the index field.the number of fields in the index is inconsistent.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16790";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "long", "string", "int", "date","string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";
   commCreateIndex( dbcl, indexName1, {'stra':1,'intb':1,'datec':1,'f':1} );
   commCreateIndex( dbcl, indexName2, {'stra':1,'intb':-1,'ld':1} );
   
   println("---test a:query fields and index fields in the same order.");
   var findCond = {'stra':{$lt:1000},'intb':{$gt:0}};
   var sortCond = {'stra':1};
   var getExplainReslut = getExplain( dbcl, findCond, sortCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 );
   
   println("---test b:query fields and index fields in different order.");
   var findCond1 = {'intb':{$gt:0},'stra':{$lt:1000}};
   var sortCond1 = {'stra':-1};
   var getExplainReslut1 = getExplain( dbcl, findCond1, sortCond1 );
   checkExplain( getExplainReslut1, "ixscan", indexName2 );
   commDropCL(db, COMMCSNAME, clName, true, true);
}

