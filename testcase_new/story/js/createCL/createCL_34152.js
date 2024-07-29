/******************************************************************************
 * @Description   : seqDB-34152:哈希分区表指定数据组不属于本domain
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   var domainName = 'domain_34152';
   var csName = 'cs_34152';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupInDomain = groupsArray[0][0].GroupName;
   var groupOutDomain = groupsArray[1][0].GroupName;
   commCreateDomain( db, domainName, [ groupInDomain ], { AutoSplit: true } );
   commCreateCS( db, csName, false, "create CS specify domain", { Domain: domainName } );
   clName = 'cl_34152';
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      commCreateCL( db, csName, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": [ groupOutDomain ] } );
   } );
   commDropCS( db, csName );
   commDropDomain( db, domainName );
}
