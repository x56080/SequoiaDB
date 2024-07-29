/******************************************************************************
 * @Description   : seqDB-34194 SplitGroupStart 指定非法参数，如字符串、对象
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupNameArray = [];
   for (var i = 0; i < groupsArray.length; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34193';
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray, "SplitGroupStart": "a" } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34193';
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray, "SplitGroupStart": -2 } );
   } );
}
