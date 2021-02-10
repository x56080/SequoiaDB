/* *****************************************************************************
@description: seqDB-22884:在使用数据源的集合空间下创建使用数据源的集合 
@author: 2020-10-20 wuyan  Init
***************************************************************************** */
main( test );
function test( )
{   
   var dataSrcName = "datasrc22884";   
   var csName = "cs_22884";
   var clName = "cl_22884";   
   var srcCSName = "datasrcCS_22884";   
   var datasrcDB = new Sdb(datasrcIp,datasrcPort,userName,passwd);    
   commDropCS ( datasrcDB, srcCSName );   
   clearDataSource(csName, dataSrcName);   
   commCreateCS ( datasrcDB, srcCSName);
   commCreateCL( datasrcDB, srcCSName, clName);      
   db.createDataSource(dataSrcName, datasrcUrl,userName,passwd);    
   
   var cs = db.createCS( csName,{DataSource:dataSrcName,Mapping: srcCSName }); 
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      cs.createCL(clName);
   }) ;
  
   db.dropCS(csName);   
   datasrcDB.dropCS(srcCSName);
   db.dropDataSource(dataSrcName); 
   datasrcDB.close();     
}


