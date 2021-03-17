/******************************************************************************
 * @Description   : seqDB-22873:源集群上删除cs
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22873";
   var csName = "cs_22873";
   var srcCSName = "datasrcCS_22873";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   var cl = db.getCS( csName ).getCL( srcCLName );
   db.dropCS( csName );

   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );

   datasrcDB.getCS( srcCSName );
   datasrcDB.dropCS( srcCSName );
   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}
