/******************************************************************************
@Description : seqDB-15546:指定AutoIndexId:false创建hash分区表，alter为自动切分
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
var groupNames = commGetDataGroupNames( db );
var srcGroupName = groupNames[0];
var dstGroupName = groupNames[1];

var dmName = CHANGEDPREFIX + "_split15546";
commDropDomain( db, dmName, true );
commCreateDomain( db, dmName, groupNames, {}, false );

testConf.csName = CHANGEDPREFIX + "_split15546";
testConf.csOpt = { "Domain": dmName };
testConf.clName = "cl";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoIndexId": false };
var recsNum = 100;

main( test );
function test ( arg )
{
   var cl = arg.testCL;

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i } );
   }
   cl.insert( docs );

   //set CL attributes
   try
   {
      cl.setAttributes( { "AutoSplit": true } );
      throw new Error( "expected failure, actual return success." );
   }
   catch( e )
   {
      if( "-279" !== e.message )
      {
         throw e;
      }
   }

   // create idIndex, and set again
   cl.createIdIndex();
   cl.setAttributes( { "AutoSplit": true } );
   checkCLAttr( cl );

   // clean
   commDropDomain( db, dmName, false, "drop cs in the end" );
}

function checkCLAttr ( cl )
{
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { "Name": testConf.csName + "." + testConf.clName } );
   var autoSplit = cursor.current().toObj().AutoSplit;
   cursor.close();
   if( autoSplit !== true )
   {
      throw new Error( "expect AutoSplit:true, AutoSplit:" + autoSplit );
   }
}