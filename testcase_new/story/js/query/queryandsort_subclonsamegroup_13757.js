
var maincl = COMMCLNAME + "_maincl"; 
var subcl1 = COMMCLNAME + "_subcl1";
var subcl2 = COMMCLNAME + "_subcl2";

function setUp(db)
{
   try
   {
      var groups = commGetGroups(db, true, "");
      for (var i = 0; i< groups.length; ++i)
      {
         if (groups[i].length > 0)
         {
            break;
         }
      } 
      
      var opt = new Object();
      var subopt = new Object();
      subopt.a = 1;
      opt.Group = groups[i][0].GroupName;
      opt.ReplSize = 0;
      var subcl1 = commCreateCLByOption(db, COMMCSNAME, subcl1, opt, ture);
      var subcl2 = commCreateCLByOption(db, COMMCSNAME, subcl1, opt, ture);
      
      opt.IsMainCL = true;
      opt.ShardingType = "range";
      opt.ShardingKey = subopt;
      
      var maincl = commCreateCLByOption(db, COMMCSNAME, maincl, opt ,ture);
     
      maincl.attachCL(COMMCSNAME + '.' + subcl1,{LowBound:{a:0},UpBound:{a:5}})
      maincl.attachCL(COMMCSNAME + '.' + subcl2,{LowBound:{a:5},UpBound:{a:10}})
      
      return maincl;
   }
   catch(e)
   {
      buildException("setUp", e);
   } 
}

function tearDown(db)
{
   try
   {
      commDropCL( db, COMMCSNAME, maincl, true, true) ;
   }
   catch(e)
   {
      throw buildException("tearDown", e);
   } 
}

function loadData(cl)
{
   try
   {
      for (var i = 0; i< 10; ++i)
      {
         cl.insert({_id:i, a:i});
      }
   }
   catch(e)
   {
      throw buildException("loadData", e);
   } 
}

function execqueryandsort(cl)
{
   try
   {
      var resultSet = cl.find().sort({a:-1}).skip(4).toArray();
      return resultSet;
   }
   catch(e)
   {
      throw buildException("execqueryandsort", e);
   }    
}

function checkResult(oper, resultSet)
{
   if (resultSet.length != 6)
   {
      throw buildException("checkResult", 0, oper, 6, resultSet.length);
   }
   
   var arr = new Array(5,4,3,2,1,0)
   for (var i = 0; i< resultSet.length; ++i)
   {
      var obj = eval("(" + resultSet[i] + ")");
      if (obj._id != obj.a && obj.a != arr[i])
      {
         var expectresult = "{_id:" + arr[i] + ",a:" + arr[i] + "}";
         var realresult = "{_id:" + obj._id + ",a:" + obj.a + "}";
         throw buildException("checkResult", 0, oper, expectresult , realresult);
      }
   }
}

function main()
{
   try
   {
      db = new Sdb();
      if (commIsStandalone(db))
      {
         return;
      }
      var cl = setUp(db);
      loadData(cl)
      var result = execqueryandsort(cl);
      checkResult("find().sort({a:-1}).skip(4)", result);
   }
   catch(e)
   {
      throw e;
   }
   finally
   {
      tearDown(db);
      db.close();
   }
}