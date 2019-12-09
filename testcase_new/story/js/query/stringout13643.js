/*******************************************************************************
*@Description:   seqDB-13643:查询不使用排序，指定selector为$include和flags=FLG_QUERY_STRINGOUT
*@Author:        2019-2-25  wangkexin
********************************************************************************/

main();
function main ()
{
    var csName = COMMCSNAME;
    var clName = "cl13643";
    //clean environment before test
    commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );
    var cl = commCreateCL( db, csName, clName );

    readyData( cl );
    //特殊数据类型如：最小值$minKey、最大值$maxKey、空值null、数组、对象、二进制数据、正则表达式会被忽略，stringout会返回"":""
    //使用$include选择符，不使用排序 指定flags=FLG_QUERY_STRINGOUT
    var rc = cl.find( {}, { Key: { $include: 1 } } ).flags( 1 );
    var expRecs = [{ "": "123" }, { "": "3000000000" }, { "": "123.456" }, { "": "{ \"$decimal\": \"123.456\" }" }, { "": "value" }, { "": "123abcd00ef12358902300ef" }, { "": "true" }, { "": "2012-01-01" }, { "": "2012-01-01-13.14.26.124233" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }];
    checkRec( rc, expRecs );
    commDropCL( db, csName, clName );
}