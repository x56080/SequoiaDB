/************************************
*@Description: seqDB-16798 multiple indexes match fields for query conditions,
               hint to index.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16798";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "string", "int", "date","long", "string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";  
   var indexName3 = "indexc";  
   commCreateIndex( dbcl, indexName1, {'stra':1,'datec':1,'stre':1} );
   commCreateIndex( dbcl, indexName2, {'stra':-1,'intb':-1,'stre':1,'f':1} );     
   commCreateIndex( dbcl, indexName3, {'stra':-1,'datec':-1,'ld':-1} );     
   
   var findCond = {'datec':{$ne:''},'stra':{$lt:1000}};    
   var hintCond = {'':indexName2};
   
   println("---no using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond );
   checkExplain( getExplainReslut, "ixscan", indexName1 );   
   
   println("---using hint to index.")
   var getExplainReslut = getExplain( dbcl, findCond, null, hintCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 ); 
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}

