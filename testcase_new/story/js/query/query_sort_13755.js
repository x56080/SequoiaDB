/*******************************************************************************
*@Description : seqDB-13755:sort指定字段名为空
*@Modify list :
*               2015-2-10  xiaojun Hu  Init  
*               2020-10-12 huangxiaoni  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13755";
testConf.useSrcGroup = true;

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   var insertNum = 100;
   idxAutoGenData( cl, insertNum );

   // 排序字段名为空字符串
   assert.tryThrow( -6, function()
   {
      cl.find().sort( { "": 1 } ).toArray();
   } );

   assert.tryThrow( -6, function()
   {
      cl.find().sort( { "": "a" } ).toArray();
   } );

   // 排序字段名有效，值无效
   assert.tryThrow( -6, function()
   {
      cl.find().sort( { "a": 0 } ).toArray();
   } );

   assert.tryThrow( -6, function()
   {
      cl.find().sort( { "a": "1" } ).toArray();
   } );
}
