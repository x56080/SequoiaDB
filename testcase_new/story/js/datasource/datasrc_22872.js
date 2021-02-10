/******************************************************************************
 * @Description   : seqDB-22872:数据源上删除已建立映射关系的cs
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.02.06
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//main( test );

function test ()
{
   var dataSrcName = "datasrc22872";
   var csName = "cs_22872";
   var srcCSName = "datasrcCS_22872";
   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   var cl = db.getCS( csName ).getCL( srcCLName );
   datasrcDB.dropCS( srcCSName );

   /* assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
    {
       cl.find().toArray();  
    }) ; 
    */
   assert.tryThrow( SDB_CAT_DATASOURCE_NOTEXIST, function()
   {
      cl.find().toArray();
   } );

   db.dropCS( csName );
   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}
