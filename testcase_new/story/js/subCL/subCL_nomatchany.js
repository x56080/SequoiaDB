
testConf.skipStandAlone = true;
main( test );

function test ()
{
   //set priority from masterNode
   db.setSessionAttr( { PreferedInstance: "M" } );

   test_range_attach_hash_upsert_basic();

}

function getSourceGroupName_alone ( COMMCSNAME, CL_Name )
{
   var cata = new Sdb( COORDHOSTNAME, CATASVCNAME );
   var allCollections = cata.SYSCAT.SYSCOLLECTIONS.find().toArray();
   var CS_CL = COMMCSNAME + "." + CL_Name;
   var GroupName = "";
   for( var i = 0; i < allCollections.length; i++ )
   {
      var eval_CL = eval( "(" + allCollections[i] + ")" );
      if( eval_CL["Name"] == CS_CL )
      {
			/*for(var j=0;j<eval_CL["CataInfo"].length;j++)
			{
				GroupName = eval_CL["CataInfo"][j]["GroupName"] ;
			}*/
         GroupName = eval_CL["CataInfo"][0]["GroupName"];
         break;
      }
   }
   return GroupName;
}
function getOtherDataGroups ( SourceGroupName )
{
   var allGroups = db.listReplicaGroups().toArray();
   var RoleGroupNumbers = 0;
   var Groups = [];
   for( var i = 0; i < allGroups.length; i++ )
   {
      var eval_node = eval( "(" + allGroups[i] + ")" );
      if( eval_node["Role"] == 0 )
      {
         if( eval_node["GroupName"] != SourceGroupName )
         {
            Groups.push( eval_node["GroupName"] );
         }
      }
   }
   return Groups;
}

function getPartition ( COMMCSNAME, CL_Name )
{
   var cata = new Sdb( COORDHOSTNAME, CATASVCNAME );
   var allCollections = cata.SYSCAT.SYSCOLLECTIONS.find().toArray();
   var CS_CL = COMMCSNAME + "." + CL_Name;
   var Partition = "";
   for( var i = 0; i < allCollections.length; i++ )
   {
      var eval_CL = eval( "(" + allCollections[i] + ")" );
      if( eval_CL["Name"] == CS_CL )
      {
         Partition = eval_CL["Partition"];
         break;
      }
   }
   return Partition;
}
function subCL_split_hash ( subcl, SourceGroupName, OtherDataGroups, Partition )
{
   var Partition_PerGroup = Partition / ( OtherDataGroups.length + 1 );
   for( var i = 0; i < OtherDataGroups.length; ++i )
   {
      var start_Partition = Math.round( Partition_PerGroup * i );
      var end_Partition = Math.round( Partition_PerGroup * ( i + 1 ) );
      subcl.split( SourceGroupName, OtherDataGroups[i], { Partition: start_Partition }, { Partition: end_Partition } );
   }
   return 0;
}

function test_range_attach_hash_upsert_basic ()// NOT Error, test mainCL'ShardingType is range and subCL's ShardingType is hash , insert's result
{
   MainCL_Name = CHANGEDPREFIX + "year";
   subCl_Name = CHANGEDPREFIX + "month";
   commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true );
   commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true );
   commDropCL( db, COMMCSNAME, MainCL_Name, true, true );
   var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
   var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1 }, ShardingType: "range", Partition: 4096, ReplSize: 0, Compressed: true, IsMainCL: true } );
   var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { b: 0 }, UpBound: { a: 50 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 50 }, UpBound: { a: 100 } } );

   var subCL = [];
   subCL.push( subCL1 );
   subCL.push( subCL2 );
   var numberOfsubCl = 2;
   for( var i = 0; i < numberOfsubCl; ++i )
   {
      var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );

      var desDataGroupName = getOtherDataGroups( sourceDataGroupName );

      var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );

      if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition ) )
      {
      }
   }

   for( var i = 0; i < 100; ++i )
   {
      mainCL.insert( { a: i, mineName: "上海矿场", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市" } } );
   }

   mainCL.upsert( { $set: { "localtion.street": "人民路12号" } }, { mineTime: "2013-06-14" } );

   var rc = mainCL.find( { localtion: { $elemMatch: { street: "人民路12号" } } } );

   var size = rc.count();
   assert.equal( 1, size );

   //	
   //	
}
