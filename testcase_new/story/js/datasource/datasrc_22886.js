/* *****************************************************************************
@description: seqDB-22886:在使用数据源的集合空间下创建使用数据源的多个集合
@author: 2020-10-20 wuyan  Init
***************************************************************************** */
main( test );
function test( testPara)
{   
   var dataSrcName = "datasrc22886";   
   var csName = "cs_22886";
   var clName = "cl_22886";   
   var srcCSName = "datasrcCS_22886";   
   var datasrcDB = new Sdb(datasrcIp,datasrcPort,userName,passwd);   
   commDropCS ( datasrcDB, srcCSName );   
   clearDataSource(csName, dataSrcName);   
   commCreateCS ( datasrcDB, srcCSName);  
   
   var clNum = 100;
   for( var i=0 ;i< clNum; i++)
   {
      var name = clName + "_datasrc_" + i;
      commCreateCL( datasrcDB, srcCSName, name, {ShardingKey:{a:1}}); 
   }     
      
   db.createDataSource(dataSrcName, datasrcUrl,userName,passwd );   
   var cs = db.createCS( csName,{DataSource:dataSrcName,Mapping: srcCSName }); 
   
   for( var i=0;i< clNum; i++)
   {
      var name = clName + "_datasrc_" + i;
      var dbcl = cs.getCL( name ); 
      insertAndfindRecords( dbcl );      
   }  
      
   db.dropCS(csName);   
   datasrcDB.dropCS(srcCSName);
   db.dropDataSource(dataSrcName); 
   datasrcDB.close();     
}

function insertAndfindRecords( dbcl )
{
   var recordNum = 2000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 4000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );    
   expRecs.sort(sortBy('a'));
   commCompareResults( cursor, expRecs );  

   var count = dbcl.count();
   assert.equal( count, recordNum  );
}
