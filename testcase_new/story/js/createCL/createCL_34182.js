/******************************************************************************
 * @Description   : seqDB-34182:普通表指定 SplitGroupStart
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName = 'cl_34182';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupsArray[0][0].GroupName;
   commCreateCL( db, COMMCSNAME, clName, {
         "Group": groupName,
         "SplitGroupStart": -1
   } );
   var result = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   var cataInfo = result.CataInfo;
   assert.equal( cataInfo.length, 1 );
   assert.equal( cataInfo[0].GroupName, groupName );
   commDropCL( db, COMMCSNAME, clName );
}
