/************************************
*@Description: seqDB-16801 multiple indexes match fields for query conditions,
               and query use the same index,hint to index.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16801";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "string", "int", "date","long", "string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";  
   commCreateIndex( dbcl, indexName1, {'stra':1,'intb':1,'datec':1} );
   commCreateIndex( dbcl, indexName2, {'stra':-1,'intb':-1,'ld':1} );     
   
   var findCond = {'intb':{$gt:0},'stra':{$lt:1000}};  
   var sortCond = {'stra':-1};
   var hintCond = {'':indexName2};
   
   println("---no using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond, sortCond );
   checkExplain( getExplainReslut, "ixscan", indexName1 );   
   
   println("---using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond, sortCond, hintCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 ); 
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}

