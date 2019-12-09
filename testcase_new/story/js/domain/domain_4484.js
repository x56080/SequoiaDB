/************************************
*@Description: createDomain，覆盖name/groups/options有效字符和边界_st.verify.domain.001
*@author:      wangkexin
*@createDate:  2019.6.5
*@testlinkCase: seqDB-4484
**************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }


   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var dataRGArr = [];
   var csNames = ["cs_4484_a", "cs_4484_b", "cs_4484_c"];
   var domainNames = ["a", "123-test-中文。~!@#%^&*()_+-=\][|,/<>?~·！@#￥%……&*~!-%^*()_-+=|\/<'?*[];:/#（）——+《》{}|【】、，。~_127B", "testAutoSplit"];

   //clean environment before test
   dropCSAndDomain( csNames, domainNames );

   for( var i = 0; i < groupsArray.length; i++ )
   {
      dataRGArr.push( groupsArray[i][0].GroupName );
   }

   //域名覆盖1字节有效字符，group个数为1，AutoSplit指定为true
   var domainDataRg = [dataRGArr[0]];
   var domain = domainNames[0];
   db.createDomain( domain, domainDataRg, { AutoSplit: true } );
   checkDomain( domain, domainDataRg, true );
   checkByCreateCS( domain, csNames[0] );

   //域名覆盖127字节，group个数为随机值，AutoSplit指定为false
   var domainDataRg2 = [];
   var domain2 = domainNames[1];
   var groupNum = Math.floor( Math.random() * dataRGArr.length ) + 1;
   for( var i = 0; i < groupNum; i++ )
   {
      domainDataRg2.push( dataRGArr[i] );
   }
   db.createDomain( domain2, domainDataRg2, { AutoSplit: false } );
   checkDomain( domain2, domainDataRg2, false );
   checkByCreateCS( domain2, csNames[1] );

   //不指定AutoSplit
   var domainDataRg3 = [dataRGArr[0]];
   var domain3 = domainNames[2];
   db.createDomain( domain3, domainDataRg3 );
   checkDomain( domain3, domainDataRg3 );
   checkByCreateCS( domain3, csNames[2] );

   //域名覆盖不为string类型
   try
   {
      db.createDomain( 1000000000000000, dataRGArr[0] );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create domain " + domainNames[i] + " failed", -6, e );
      }
   }

   dropCSAndDomain( csNames, domainNames );
}

function checkDomain ( domain, datagroups, autosplit )
{
   var actGroups = [];
   var cur = db.listDomains( { "Name": domain } );
   var actAutoSplit = null;
   if( cur.next() )
   {
      var groups = cur.current().toObj()["Groups"];
      if( typeof ( autoSplit ) !== undefined )
      {
         actAutoSplit = cur.current().toObj()["AutoSplit"];
      }
      for( var i = 0; i < groups.length; i++ )
      {
         actGroups.push( groups[i].GroupName );
      }
   }
   else
   {
      throw "can not find domain : " + domain;
   }

   actGroups.sort();
   datagroups.sort();
   if( JSON.stringify( datagroups ) !== JSON.stringify( actGroups ) )
   {
      throw buildException( "checkDomain()", null, "groups is wrong", JSON.stringify( datagroups ), JSON.stringify( actGroups ) );
   }
   if( autosplit !== actAutoSplit )
   {
      throw buildException( "checkDomain()", null, "autoSplit value is wrong", autosplit, actAutoSplit );
   }
}

function dropCSAndDomain ( csNames, domainNames )
{
   for( var i = 0; i < csNames.length; i++ )
   {
      commDropCS( db, csNames[i], true, "drop cs " + csNames[i] );
   }

   for( var i = 0; i < domainNames.length; i++ )
   {
      try
      {
         db.dropDomain( domainNames[i] );
      }
      catch( e )
      {
         if( -214 !== e )
         {
            throw buildException( "dropDomain()", e, "drop domain " + domainNames[i] + " failed", -214, e );
         }
      }
   }
}

function checkByCreateCS ( domain, csName )
{
   db.createCS( csName, { Domain: domain } );
   try
   {
      db.getCS( csName );
   }
   catch( e )
   {
      throw "get cs " + csName + "failed, e = " + e;
   }
}