/******************************************************************************
 * @Description   : seqDB-34184:主表指定 SplitGroupStart
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
   var clName = 'cl_34184';
   commCreateCL( db, COMMCSNAME, clName, {
         "ShardingKey": { "a": 1 },
         "ShardingType": "range",
         "IsMainCL": true,
         "SplitGroupStart": -1
   } );
   commDropCL( db, COMMCSNAME, clName );
}
