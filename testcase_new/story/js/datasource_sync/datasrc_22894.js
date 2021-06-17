/******************************************************************************
 * @Description   : seqDB-22894:设置会话属性访问数据源，其中数据源配置会话属性不同
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2021.06.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc22894";
   var clName = CHANGEDPREFIX + "_datasource22894";
   var srcCSName = "datasrcCS_22894";
   var csName = "DS_22894";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );  
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } , ReplSize: 0} );
   var groupNames = commGetCLGroups( datasrcDB, srcCSName + "." + clName );
   var groupName = groupNames[0];

   var cs = db.createCS( csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );

   var instanceid = 12;
   var node = datasrcDB.getRG( groupName ).getSlave();

   try
   {
      var instanceidConf = 200;
      updateConf( datasrcDB, { instanceid: instanceid, PreferedInstance: instanceidConf }, { NodeName: node.toString() }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      var options = { PreferedInstance: instanceid };
      var expAccessNodes = [ node.toString() ];
      checkAccessNodes( dbcl, expAccessNodes, options );
   }
   finally
   {
      deleteConf( datasrcDB, { instanceid: 1, PreferedInstance: 1 }, { NodeName: node.toString() }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   commDropCL( db, csName, clName, false, false );
   clearDataSource( csName, dataSrcName );
}

