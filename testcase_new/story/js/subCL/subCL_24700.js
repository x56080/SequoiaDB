/*****************************************************************************
@Description : seqDB-24700:主子表逆序排序也能利用主子表顺序（主表的分区键设置多字段）
@Author      : xiaozhenfan
@CreateTime  : 2021.12.1
@LastEditTime: 2021.12.6
@LastEditors : xiaozhenfan
******************************************************************************/
testConf.skipStandAlone = true;
function setup ( testPara )
{
    dataGroupNames = commGetDataGroupNames( db ) ;
    testPara.csName = "cs_24700" ;
    testPara.mainCLName = "maincl_24700" ;
    testPara.mainCLOpt = { IsMainCL:true, ShardingKey:{a:1,b:-1} } ;
    testPara.subCLName1 = "subcl1_24700" ;
    testPara.subCLName2 = "subcl2_24700" ;
    testPara.subCLOpt = { Group:dataGroupNames[0] } ;
    testPara.subCLFullName1 = "cs_24700.subcl1_24700" ;
    testPara.subCLFullName2 = "cs_24700.subcl2_24700" ;
    testPara.subCLFullOpt1 = { LowBound:{a:0,b:1000},UpBound:{a:0,b:0} } ;
    testPara.subCLFullOpt2 = { LowBound:{a:1000,b:1000},UpBound:{a:2000,b:0} } ;
    commDropCS( db, testPara.csName ) ;
}
main( setup, test ) ;
function test( testPara )
{
    commCreateCS ( db, testPara.csName ) ;
    var mainCL = commCreateCL( db, testPara.csName, testPara.mainCLName, testPara.mainCLOpt ) ; 
    commCreateCL( db, testPara.csName, testPara.subCLName1, testPara.subCLOpt ) ;
    commCreateCL( db, testPara.csName, testPara.subCLName2, testPara.subCLOpt ) ;
    mainCL.attachCL( testPara.subCLFullName1, testPara.subCLFullOpt1 ) ;
    mainCL.attachCL( testPara.subCLFullName2, testPara.subCLFullOpt2 ) ;
    var needReorder = mainCL.find({},{}).sort({a:-1}).explain({Detail:true,Expand:true})
                      .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:1}).explain({Detail:true,Expand:true})
                      .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;  
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:-1,b:1}).explain({Detail:true,Expand:true})
                       .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;  
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:1,b:1}).explain({Detail:true,Expand:true})
                       .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;  
    assert.equal( needReorder, true ) ;
    needReorder = mainCL.find({},{}).sort({a:1,b:-1}).explain({Detail:true,Expand:true})
                      .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;  
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:-1,b:-1}).explain({Detail:true,Expand:true})
                       .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;  
    assert.equal( needReorder, true ) ;
}
