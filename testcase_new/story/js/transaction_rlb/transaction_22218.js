/******************************************************************************
<<<<<<< HEAD
@Description :   seqDB-22218:停止1个备节点，开启事务，往replSize=0的集合中插入记录
@author :   2020-5-25    zhaoyu  Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

var groups = commGetGroups( db );
for( var i = 0; i < groups.length; i++ )
{
   var group = groups[i];
   if( group.length > 2 )
   {
      break;
   }
}
var groupName = group[0].GroupName;
var rg = db.getRG( groupName );
var primaryPos = group[0].PrimaryPos;
var slavePos = ( primaryPos === 1 ? primaryPos + 1 : 1 );
var hostName = group[slavePos]["HostName"];
var svcName = group[slavePos]["svcname"];
println( "nodeName: " + hostName + ":" + svcName );
var slaveNode = rg.getNode( hostName, svcName );
testConf.clName = CHANGEDPREFIX + "_22218";
testConf.clOpt = { ReplSize: 0, Group: groupName };

main( test );

function test ( testPara )
{
=======
 * @Description   : seqDB-22218:停止1个备节点，开启事务，往replSize=0的集合中插入记录
 * @Author        : zhaoyu
 * @CreateTime    : 2020.05.25
 * @LastEditTime  : 2022.02.16
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ( testPara )
{
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; i++ )
   {
      var group = groups[i];
      if( group.length > 2 )
      {
         break;
      }
   }
   var groupName = group[0].GroupName;
   var rg = db.getRG( groupName );
   var primaryPos = group[0].PrimaryPos;
   var slavePos = ( primaryPos === 1 ? primaryPos + 1 : 1 );
   var hostName = group[slavePos]["HostName"];
   var svcName = group[slavePos]["svcname"];
   println( "nodeName: " + hostName + ":" + svcName );
   var slaveNode = rg.getNode( hostName, svcName );
   var clName = CHANGEDPREFIX + "_22218";
   var clOpt = { ReplSize: 0, Group: groupName };

   var coll = commCreateCL( db, COMMCSNAME, clName, clOpt );

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   db.transBegin();
   try
   {
      slaveNode.stop();
<<<<<<< HEAD
      testPara.testCL.insert( { a: 1 } );
=======
      coll.insert( { a: 1 } );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      throw new Error( "need throw error: SDB_CLS_NODE_NOT_ENOUGH" );
   } catch( e )
   {
      if( SDB_CLS_NODE_NOT_ENOUGH != parseInt( e.message ) )
      {
         println( "e:" + e );
         throw e;
      }

   } finally
   {
      slaveNode.start();
      db.transRollback();
   }

<<<<<<< HEAD
   checkGroupBusiness( 120, COMMCSNAME, testConf.clName );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, [] );
=======
   checkGroupBusiness( 120, COMMCSNAME, clName );

   var cursor = coll.find();
   commCompareResults( cursor, [] );
   commDropCL( db, COMMCSNAME, clName );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}

