/*******************************************************************************
*@Description: JavaScript function main template
*@Modify list:
*   2019-11-12 wenjing Wang  Init
*******************************************************************************/

/*
入参配置项：
testConf.skipStandAlone = true;                 跳过独立模式
testConf.skipOneGroup = true;                   跳过只有一个组的环境
testConf.skipOneDuplicatePerGroup = true;       跳过每组一个节点的环境
testConf.useSrcGroup = true;                    存储创建的 cl 所在组
testConf.useDstGroup = true;                    存储创建的 cl 不在的组
testConf.csName  = COMMCSNAME + "_xxx";         指定框架创建的 cs 名
testConf.csOpt = {};                            指定创建的 cs 配置项
testConf.clName = COMMCLNAME = "_xxx";          指定框架创建的 cl 名
testConf.clOpt = {};                            指定创建的 cl 配置项

出参：
function test(testPara) {}
testPara.groups           获取数据组的信息，没有前置要求，可直接获取
testPara.testCS           获取创建的 cs，需要指定 testConf.csName
testPara.testCL           获取创建的 cl，需要指定 testConf.clName
testPara.srcGroupName     获取创建的 cl 所在组，需要指定 testConf.clName,testConf.clOpt,testConf.useSrcGroup = true
testPara.dstGroupNames    获取创建的 cl 不在的组，需要指定 testConf.clName,testConf.clOpt,testConf.useDstGroup = true
*/

var testConf = {
   skipStandAlone: false, skipOneDuplicatePerGroup: false,
   skipOneGroup: false, useSrcGroup: false, useDstGroup: false
};
// e.g. testConf.csName = COMMCSNAME, testConf.csOpt = {PageSize:4096}} };
// e.g. testConf.clName = COMMCLNAME, testConf.clOpt = {AutoSplit:true} } ;
// e.g. testConf.useSrcGroup = true  返回源组，一般用于创建CL时指定组；设置true后在测试方法中获取源组，如test( arg ){ arg.srcGroupName ...}
// e.g. testConf.useDstGroup = true  返回目标组，一般用于切分；设置为true返回除源组外的所有组，在测试方法中获取源组，如test( arg ){ arg.dstGroupNames ...}

var testPara = {};

var oneGroup = 1;
var nodeNum = 1;
function checkEnv ( db, testConf )
{
   if( testConf.skipStandAlone && commIsStandalone( db ) )
   {
      throw new Error( "standalone" );
   }

   testPara.groups = commGetGroups( db );
   if( testConf.skipOneGroup )
   {
      if( testPara.groups.length === oneGroup )
      {
         throw new Error( "one data group" );
      }
   }

   if( testConf.skipOneDuplicatePerGroup )
   {
      for( var i = 0; i < testPara.groups.length; ++i )
      {
         if( testPara.groups[i].length - 1 > nodeNum )
         {
            break;
         }
      }

      if( i === testPara.groups.length )
      {
         throw new Error( "one duplicate per group" );
      }
   }
}

function buildDomainContainGroups ()
{
   if( testConf.DomainUseGroupNum === undefined ) { testConf.DomainUseGroupNum === testPara.groups.length }
   var dmGroupNames = [];
   for( var i = 0; i < testPara.groups.length; ++i )
   {
      var groupName = testPara.groups[i][0].GroupName;
      if( groupName !== CATALOG_GROUPNAME && groupName !== COORD_GROUPNAME && groupName !== SPARE_GROUPNAME )
      {
         dmGroupNames.push( testPara.groups[i][0].GroupName );
      }

      if( dmGroupNames.length == testConf.DomainUseGroupNum )
      {
         break;
      }
   }
   return dmGroupNames;
}

var dataGroupNames = [];
function getAllDataGroupName ()
{
   if( dataGroupNames.length !== 0 )
   {
      return dataGroupNames;
   }

   for( var i = 0; i < testPara.groups.length; ++i )
   {
      var groupName = testPara.groups[i][0].GroupName;
      if( groupName !== CATALOG_GROUPNAME && groupName !== COORD_GROUPNAME
         && groupName !== SPARE_GROUPNAME )
      {
         dataGroupNames.push( groupName );
      }
   }

   return dataGroupNames;
}

function getSrcGroupName ()
{
   var dataGroupNames = getAllDataGroupName();
   var pos = Math.floor( Math.random() * dataGroupNames.length );
   return dataGroupNames[pos];
}

function getDstGroupName ( srcGroupName )
{
   var groupNames = [];
   var dataGroupNames = getAllDataGroupName();
   for( var i = 0; i < dataGroupNames.length; ++i )
   {
      if( dataGroupNames[i] !== srcGroupName )
      {
         groupNames.push( dataGroupNames[i] );
      }
   }
   return groupNames;
}

function createTestCS ( db, testConf )
{
   if( testConf.csName !== undefined )
   {
      if( testConf.csOpt !== undefined )
      {
         if( testConf.csOpt.Domain !== undefined )
         {
            testPara.dmGroupNames = buildDomainContainGroups();
            commCreateDomain( db, testConf.csOpt.Domain, testPara.dmGroupNames, testConf.domainOpt, true );
         }
         return commCreateCS( db, testConf.csName, true, "", testConf.csOpt );
      }
      else
      {
         return commCreateCS( db, testConf.csName, true );
      }
   }
   else
   {
      testConf.csName = COMMCSNAME;
   }
}

function createTestCL ( db, testConf )
{
   if( testConf.clName !== undefined )
   {
      if( testConf.clName !== COMMCLNAME )
      {
         commDropCL( db, testConf.csName, testConf.clName, true, true );
      }

      if( testConf.clOpt !== undefined )
      {
         if( testConf.useSrcGroup )
         {
            testPara.srcGroupName = getSrcGroupName();
            testConf.clOpt.Group = testPara.srcGroupName;
         }

         if( testConf.useDstGroup )
         {
            testPara.dstGroupNames = getDstGroupName( testPara.srcGroupName );
         }
         return commCreateCL( db, testConf.csName, testConf.clName, testConf.clOpt, true, true );
      }
      else
      {
         return commCreateCL( db, testConf.csName, testConf.clName );
      }
   }
}

function dropTestCS ( db, testConf )
{
   if( testConf.csName !== undefined &&
      testConf.csName !== COMMCSNAME )
   {
      commDropCS( db, testConf.csName, true );

      if( testConf.skipStandAlone !== true && testConf.csOpt !== undefined && testConf.csOpt.Domain !== undefined )
      {
         commDropDomain( db, testConf.csOpt.Domain, true );
      }
   }
}

function dropTestCL ( db, testConf )
{
   if( testConf.clName !== undefined &&
      testConf.clName !== COMMCLNAME &&
      testConf.csName === COMMCSNAME )
   {
      commDropCL( db, testConf.csName, testConf.clName, true, true );
   }
}

function commonSetUp ( db, testConf )
{
   checkEnv( db, testConf );

   testPara.testCS = createTestCS( db, testConf );
   testPara.testCL = createTestCL( db, testConf );
}

function commonTearDown ( db, testConf )
{
   if( db !== undefined )
   {
      dropTestCL( db, testConf );
      dropTestCS( db, testConf );
      db.close();
   }
}

function main ()
{
   try
   {
      commonSetUp( db, testConf );
      var numArgs = arguments.length;
      for( var i = 0; i < numArgs; ++i )
      {
         if( typeof ( arguments[i] ) === "function" )
         {
            arguments[i]( testPara );
         }
      }
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         if( e.message === "standalone" ||
            e.message === "one data group" ||
            e.message === "one duplicate per group" )
         {
            return;
         }
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      commonTearDown( db, testConf );
   }
}
