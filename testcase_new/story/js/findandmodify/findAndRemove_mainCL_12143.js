/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-8 wenjing wang  Init
*******************************************************************************/

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed( cl )
{
   var funname = "test_UsedSkipOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).skip( 2 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).skip( 2 ).remove()"; 
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
*@Input：find( {date:{$gte:20150101}} ).limit( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed( cl )
{
   var funname = "test_UsedLimitOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).limit( 2 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).limit( 2 ).remove()"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip + limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed( cl )
{
   var funname = "test_UsedSkipAndLimitOfFailed"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).remove()"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}


/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).remove()
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
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lte:20150201}}]} ).skip( 2 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).remove()"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合limit，结果不落在单个分区表上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).remove()
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
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).remove() )"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip + limit，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailedSplit( cl )
{
   var funname = "test_UsedSkipAndLimitOfFailedSplit"; 
   try
   {
      var groupNum = getDataGroupNum(); 
      if( groupNum == 1 ) return; 
      loadMultipleDoc( cl, 5 * 4 * groupNum ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lte:20150201}}]} ).skip( 2 ).limit( 5 ).remove().toArray(); 
      
      throw -1; 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).remove()"; 
         throw buildException( funname, e, oper, errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

function checkResult( cl, totalnum, subnum )
{
   try
   {
      var totalcount = cl.find().count(); 
      var subcl0count = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).count(); 
      if( totalnum != parseInt( totalcount )&& subnum != parseInt( subcl0count ) )
      {
         throw -1; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         if( totalnum == totalcount )
         {
            oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).count()"
            throw buildException( "checkResult", e, oper, subnum, subclcount ); 
         }
         else
         {
            throw buildException( "checkResult", e, "find().count()", totalnum, totalcount ); 
         }
      }
      else
      {
         throw buildException( "checkResult", e ); 
      }
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 5 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipOfSuccessNonSplit( cl )
{
   var funname = "test_UsedSkipOfSuccessNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 1 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      
      checkResult( cl, 16, 1 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 5 ).remove()"; 
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
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用limit( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedLimitOfSuccessNonSplit( cl )
{
   var funname = "test_UsedLimitOfSuccessNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      
      checkResult( cl, 15, 0 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).remove()"; 
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
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用limit + skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessNonSplit( cl )
{
   var funname = "test_UsedSkipAndLimitOfSuccessNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 5 * 4 ); 
      var arrdoc = cl.find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, 18, 3 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).remove()"; 
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
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 1 ).remove()
*@Expectation：返回的文档已经被删除
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
      skip( 1 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, totalnum -4, 1 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}, {_id:{$gte:0}}" + 
         ", {_id:{$lt:5}}]} ).skip( 1 ).remove()"
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
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用limit( 结果落在分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
limit( 5 ).remove()
*@Expectation：返回的文档已经被删除
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
      limit( 5 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, totalnum - 5, 0 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, " + 
         "date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).limit( 5 ).remove()"
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
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用limit + skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 2 ).limit( 2 ).remove()
*@Expectation：返回的文档已经被删除
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
      skip( 2 ).limit( 2 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
      checkResult( cl, totalnum - 2, 3 ); 
   }
   catch( e )
   {
      if( -1 == e )
      {
         var oper = "find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}, " + 
         "{_id:{$gte:0}}, {_id:{$lt:5}}]} ).skip( 2 ).limit( 2 ).remove()"; 
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
   catch( e )
   {
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
      if( datagroups > 1 )
      {
         init( db, maincl, false, datagroups[0][0].GroupName ); 
         test_UsedSkipOfFailed( maincl ); 
         test_UsedLimitOfFailed( maincl ); 
         test_UsedSkipAndLimitOfFailed( maincl ); 
         test_UsedSkipOfSuccessNonSplit( maincl ); 
         test_UsedLimitOfSuccessNonSplit( maincl ); 
         test_UsedSkipAndLimitOfSuccessNonSplit( maincl ); 
      }
   }
   catch( e )
   {
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
      var replsize = 1; 
      var db = setUp( replsize, createMode.vertical ); 
      if( commIsStandalone( db ) )
      {
         return; 
      }
      
      var maincl = getCL( db ); 
      db.setSessionAttr( {PreferedInstance:"M"} ); 
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
