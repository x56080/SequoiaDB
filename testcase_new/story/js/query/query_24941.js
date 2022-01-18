/******************************************************************************
 * @Description   : seqDB-24941:删除主集合后新建同名普通集合，从另一个协调节点立刻插入数据
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.8
 * @LastEditTime  : 2022.01.18
 * @LastEditors   : 钟子明
 ******************************************************************************/

test();

function test () 
{
   //进行检测，如果是standalone或者协调节点数量小于2则跳过用例
   if( commIsStandalone( db ) || skipCoordlessthan2( db ) ) 
   {
      return;
   }
   var csName = COMMCSNAME + "_24941"
   var clName = COMMCLNAME + "_24941";

   commCreateCS( db, csName, true );
   var groupName = getOneDataGroup( db );
   createCL( db, groupName );
   var data = prepareData();

   var db2 = getAnotherCoord( db );

   println( '-- begin test' );
   commDropCL( db, csName, clName );
   db.getCS( csName ).createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   println( '-- begin to insert data' );
   db2.getCS( csName ).getCL( clName ).insert( data );
   println( '-- end test' );

   db2.close();

   commDropCS( db, csName );
}

function createCL ( db1, groupName )
{
   var cl = db1.getCS( csName ).createCL( 'newcl1', { Group: groupName, ShardingKey: { a: 1 } } );
   cl = db1.getCS( csName ).createCL( 'newcl2', { Group: groupName, ShardingKey: { a: 1 } } );
   var maincl = db1.getCS( csName ).createCL( clName, { IsMainCL: true, ShardingKey: { b: 1 } } );
   maincl.attachCL( csName + '.newcl1', { LowBound: { b: 0 }, UpBound: { b: 5000 } } );
   maincl.attachCL( csName + '.newcl2', { LowBound: { b: 5000 }, UpBound: { b: 10000 } } );
}

function prepareData () 
{
   var data = [];

   for( i = 0; i < 10000; i++ ) 
   {
      data.push( { a: i, b: i } );
   }
   return data;
}

function getOneDataGroup ( db1 )
{
   var array = db1.listReplicaGroups( {} ).toArray();
   for( var i = 0; i < array.length; i++ )
   {
      var group = JSON.parse( array[i] );
      if( group.Role == 0 )
      {
         var groupName = group.GroupName;
         println( 'get a data group' );
         return groupName;
      }
   }
   println( "don't get a data group" );
}

function skipCoordlessthan2 ( db1 ) 
{
   return JSON.parse( db1.getCoordRG().getDetail()[0] ).Group.length < 2;
}

function getAnotherCoord ( db1 )
{
   var db2 = db1.getCoordRG().getSlave();
   while( db1.toString() == db2.toString() )
   {
      db2 = db1.getCoordRG().getSlave();
   }
   return db2.connect();
}