/************************************
*@Description: seqDB-16800 multiple indexes match fields for query conditions,
               the sort field matches the index field,hint to index.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16800";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "long", "string", "int", "date","string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";  
   commCreateIndex( dbcl, indexName1, {'stra':1,'intb':1,'datec':1,'f':1} );
   commCreateIndex( dbcl, indexName2, {'stra':-1,'intb':-1,'ld':1} );     
   
   var findCond = {'stra':{$lt:1000},'intb':{$gt:0}};
   var sortCond = {'stra':-1};
   var hintCond = {'':indexName1};
   
   println("---no using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond, sortCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 );   
   
   println("---using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond, sortCond, hintCond );
   checkExplain( getExplainReslut, "ixscan", indexName1 ); 
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}

