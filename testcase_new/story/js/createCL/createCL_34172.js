/******************************************************************************
 * @Description   : seqDB-34172:RefObj / RefFrom 引用其它 domain 表
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
   var domainName = 'domain_34172';
   var csName = 'cs_34172';
   var normalCLName = 'normal_34172';
   var cl = commCreateCL( db, COMMCSNAME, normalCLName, {
      ConsistencyStrategy: 2,
      AutoIndexId: false,
      StrictDataMode: false } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   var normalCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + normalCLName } ).next().toObj();

   var groupOutDomain = normalCLObj.CataInfo[0].GroupName;
   var groupInDomain = [];
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   for (var i = 0; i < groupsArray.length; i++)
   {
      var groupName = groupsArray[i][0].GroupName;
      if (groupName != groupOutDomain)
      {
         groupInDomain.push( groupName );
      }
   }
   commCreateDomain( db, domainName, groupInDomain, { AutoSplit: true } );
   commCreateCS( db, csName, false, "create CS specify domain", { Domain: domainName } );
   clName = 'cl_34172';
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      commCreateCL( db, csName, clName, { RefFrom: COMMCSNAME + "." + normalCLName } );
   } );
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      commCreateCL( db, csName, clName, { RefObj: normalCLObj } );
   } );
   commDropCS( db, csName );
   commDropDomain( db, domainName );
}
