/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-8 wenjing wang  Init
*******************************************************************************/

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed( cl )
{
   var funname = "test_UsedSkipOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).skip( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).skip( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed( cl )
{
   var funname = "test_UsedLimitOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).limit( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).limit( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip + limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed( cl )
{
   var funname = "test_UsedSkipAndLimitOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )"
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}


/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip, 子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailedSplit( cl )
{
   var funname = "test_UsedSkipOfFailedSplit"; 
   try
   {
      var groupNum = getDataGroupNum(); 
      if( groupNum == 1 ) return; 
      loadMultipleDoc( cl, 5 * 4 * groupNum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合limit，子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailedSplit( cl )
{
   var funname = "test_UsedLimitOfFailedSplit"; 
   try
   {
      var groupNum = getDataGroupNum(); 
      if( groupNum == 1 ) return; 
      loadMultipleDoc( cl, 5 * 4 * groupNum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip + limit，子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailedSplit( cl )
{
   var funname = "test_UsedSkipAndLimitOfFailedSplit"; 
   try
   {
      var groupNum = getDataGroupNum(); 
      if( groupNum == 1 ) return; 
      loadMultipleDoc( cl, 5 * 4 * getDataGroupNum() ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} ).toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

function checkResult( cl, updatenum )
{
   try
   {
      var updatecount = cl.find( {b:1} ).count(); 
      println( "find( {b:1} ) number" + updatecount ); 
      if( updatenum != parseInt( updatecount ) )
      {
         throw -1; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {b:1} )"; 
         throw buildException( "checkResult", e, oper, updatenum, updatecount ); 
      }
      else
      {
         throw buildException( "checkResult", e ); 
      }
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 1 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccessNonSplit( cl )
{
   var funname = "test_UsedSkipOfSuccessNonSplit"; 
   try
   {
      
      loadMultipleDoc( cl, 4 * 5 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 1 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      
      checkResult( cl, 4 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 1 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用limit( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccessNonSplit( cl )
{
   var funname = "test_UsedLimitOfSuccessNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 5 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用limit + skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessNonSplit( cl )
{
   var funname = "test_UsedSkipAndLimitOfSuccessNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 2 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}


/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 1 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccessSplit( cl )
{
   var funname = "test_UsedSkipOfSuccessSplit"; 
   try
   {
      var totalnum = 5 * 4 * getDataGroupNum(); 
      loadMultipleDoc( cl, totalnum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, 
      {date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
      skip( 1 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 4 ); 
      
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}, {_id:{$gte:0}}" + 
         ", {_id:{$lt:5}}]} ).skip( 1 ).update( {$set:{b:1}} )"
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用limit( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccessSplit( cl )
{
   var funname = "test_UsedLimitOfSuccessSplit"; 
   try
   {
      var totalnum = 5 * 4 * getDataGroupNum(); 
      loadMultipleDoc( cl, totalnum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, 
      {date:{$lt:20150201}}, 
      {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
      limit( 5 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 5 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, " + 
         "date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).limit( 5 ).update( {$set:{b:1}} )"
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用limit + skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 2 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessSplit( cl )
{
   var funname = "test_UsedSkipAndLimitOfSuccessSplit"; 
   try
   {
      var totalnum = 5 * 4 * getDataGroupNum(); 
      loadMultipleDoc( cl, totalnum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, 
      {date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
      skip( 2 ).limit( 2 ).update( {$set:{b:1}} ).toArray(); 
      
      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 2 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}, " + 
         "{_id:{$gte:0}}, {_id:{$lt:5}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} )"; 
         throw buildException( funname, e, oper, true, false ); 
      }
      else if( "string" == typeof( e ) )
      {
         throw e; 
      }
      else
      {
         throw buildException( funname, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}


function test_subclnonsplit( db, maincl )
{
   try
   {
      init( db, maincl )
      test_UsedSkipOfFailed( maincl ); 
      test_UsedLimitOfFailed( maincl ); 
      test_UsedSkipAndLimitOfFailed( maincl ); 
      test_UsedSkipOfSuccessNonSplit( maincl ); 
      test_UsedLimitOfSuccessNonSplit( maincl ); 
      test_UsedSkipAndLimitOfSuccessNonSplit( maincl ); 
   }
   catch( e )
   {
      println( "subcl non split" ); 
      throw e; 
   }
   finally
   {
      fini( db ); 
   }
}

function test_subclsplit( db, maincl )
{
   try
   {
      init( db, maincl, true ); 
      if( dataGroupNum > 1 )
      {
         test_UsedSkipOfFailed( maincl ); 
         test_UsedLimitOfFailed( maincl ); 
         test_UsedSkipAndLimitOfFailed( maincl ); 
         test_UsedSkipOfFailedSplit( maincl ); 
         test_UsedLimitOfFailedSplit( maincl ); 
         test_UsedSkipAndLimitOfFailedSplit( maincl ); 
         test_UsedSkipOfSuccessSplit( maincl ); 
         test_UsedLimitOfSuccessSplit( maincl ); 
         test_UsedSkipAndLimitOfSuccessSplit( maincl ); 
      }
   }
   catch( e )
   {
      println( "subcl split" ); 
      throw e; 
   }
   finally
   {
      fini( db ); 
   }
}

function test_subclonsamegroup( db, maincl )
{
   try
   {
      var datagroups = commGetGroups( db, true ); 
      init( db, maincl, false, datagroups[0][0].GroupName ); 
      test_UsedSkipOfFailed( maincl ); 
      test_UsedLimitOfFailed( maincl ); 
      test_UsedSkipAndLimitOfFailed( maincl ); 
      test_UsedSkipOfSuccessNonSplit( maincl ); 
      test_UsedLimitOfSuccessNonSplit( maincl ); 
      test_UsedSkipAndLimitOfSuccessNonSplit( maincl ); 
   }
   catch( e )
   {
      println( "subcl on the same group" ); 
      throw e; 
   }
   finally
   {
      fini( db ); 
   }
}

function main()
{
   try
   {
      var replsize = 0; 
      var db = setUp( replsize, createMode.vertical ); 
      if( commIsStandalone( db ) )
      {
         return; 
      }
      var maincl = getCL( db ); 
      
      db.setSessionAttr( {PreferedInstance:"M"} )
      test_subclnonsplit( db, maincl ); 
      test_subclsplit( db, maincl ); 
      test_subclonsamegroup( db, maincl ); 
   }
   catch( e )
   {
      throw e; 
   }
   finally
   {
      tearDown( db ); 
   }
}

main(); 
