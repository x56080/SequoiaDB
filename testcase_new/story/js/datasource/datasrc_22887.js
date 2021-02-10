/* *****************************************************************************
@description: seqDB-22887:数据源正在使用，源集群上删除使用数据源cl
@author: 2020-10-20 wuyan  Init
***************************************************************************** */
//main( test );

function test( )
{   
   var dataSrcName = "datasrc22887";
   var csName = "cs_22887";
   var srcCSName = "datasrcCS_22887";
   var clName = "cl_22887";
   var datasrcDB = new Sdb(datasrcIp,"11810","sdbadmin","sdbadmin");
   commDropCS ( datasrcDB, srcCSName );
   clearDataSource(csName, dataSrcName);
   commCreateCS ( datasrcDB, srcCSName);
   commCreateCL( datasrcDB, srcCSName, srcCLName);
   db.createDataSource(dataSrcName, datasrcUrl,"sdbadmin","sdbadmin")
   var cs = db.createCS(csName);  
   cs.createCL(clName,{DataSource:dataSrcName,Mapping:srcCSName + "." + clName});   
   
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.drop(csName);
   }) ;  
   
   datasrcDB.getCS(srcCSName);
   datasrcDB.dropCS(srcCSName);
   db.dropDataSource(dataSrcName);    
   datasrcDB.close();   
}
