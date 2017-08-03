/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-6 wenjing wang  Init
*******************************************************************************/

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip，结果不落在单个分区上
*@Input：find().skip(5).update({$set:{b:1}})
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed(cl)
{
   var funname = "test_UsedSkipOfFailed";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find().skip(5).update({$set:{b:1}}).toArray();
      
      throw -1;
   }
   catch(e)
   {
      if (errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e)
      {
         throw buildException(funname, e, "find().skip(5).update({$set:{b:1}})", errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用limit，结果不落在单个分区上
*@Input：find().limit(5).update({$set:{b:1}})
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed(cl)
{
   var funname = "test_UsedLimitOfFailed";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find().limit(5).update({$set:{b:1}}).toArray();
      
      throw -1;
   }
   catch(e)
   {
      if (errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e)
      {
         throw buildException(funname, e,"find().skip(5).update({$set:{b:1}})", errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip+limit,结果不落在单个分区上
*@Input：find().skip(2).limit(5).update({$set:{b:1}})
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed(cl)
{
   var funname = "test_UsedSkipAndLimitOfFailed";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find().skip(2).limit(5).update({$set:{b:1}}).toArray();
      
      throw -1;
   }
   catch(e)
   {
      if (errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES != e)
      {
         throw buildException(funname, e, "find().skip(5).limit(5).update({$set:{b:1}})", errCode.SDB_RTN_QUERYMODIFY_MULTI_NODES, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip,结果落在单个分区上
*@Input：find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).limit(2).update({$set:{b:1}})
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccess(cl)
{
   var funname = "test_UsedLimitOfSuccess";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).update({$set:{b:1}}).toArray();
      
      if (!checkUpdateResult(cl, arrdoc))
      {
         throw -1;
      }
   }
   catch(e)
   {
      if (-1 == e)
      {
         throw buildException(funname, e, "find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).update({$set:{b:1}})", true, false);
      }
      else
      {
         throw buildException(funname, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用limit,结果落在单个分区上
*@Input：find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).update({$set:{b:1}})
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccess(cl)
{
   var funname = "test_UsedSkipOfSuccess";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).update({$set:{b:1}}).toArray();
      
      if (!checkUpdateResult(cl, arrdoc))
      {
         throw -1;
      }
   }
   catch(e)
   {
      if (-1 == e)
      {
         throw buildException(funname, e, "find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).update({$set:{b:1}})", true, false);
      }
      else
      {
         throw buildException(funname, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip+limit,结果落在单个分区上
*@Input：find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).limit(2).update({$set:{b:1}})
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccess(cl)
{
   var funname = "test_UsedSkipAndLimitOfSuccess";
   try
   {
      var loadnumber = 5 * getDataGroupNum();
      loadMultipleDoc(cl, loadnumber);
      var arrdoc = cl.find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).limit(2).update({$set:{b:1}}).toArray();
      
      if (!checkUpdateResult(cl, arrdoc))
      {
         throw -1;
      }
   }
   catch(e)
   {
      if (-1 == e)
      {
         throw buildException(funname, e, "find({$and:[{_id:{$lt:5}},{_id:{$gte:0}}]}).skip(2).limit(2).update({$set:{b:1}})", true, false);
      }
      else
      {
         throw buildException(funname, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

function main()
{
   try
   {
      var replsize = 1;
      
      var db = setUp(replsize, createMode.horizontal);
      if (true != commIsStandalone(db) && dataGroupNum > 1)
      {
         db.setSessionAttr({PreferedInstance:"M"});
         var cl = getCL(db);
         test_UsedSkipOfFailed(cl);
         test_UsedLimitOfFailed(cl);
         test_UsedSkipAndLimitOfFailed(cl);
         test_UsedSkipOfSuccess(cl);
         test_UsedLimitOfSuccess(cl);
         test_UsedSkipAndLimitOfSuccess(cl);
      }  
   }
   catch(e)
   {
      throw e;
   }
   finally
   {
      tearDown(db);
   }
}

main()