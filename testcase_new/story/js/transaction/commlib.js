/*******************************************************************************
@Description : 比较查询返回的结果（游标）与预期结果( 数组 )是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( rc, expRecs )
{
   try
   {
      //get actual records to array
      var actRecs = [];
      while( rc.next() )
      {
         actRecs.push( rc.current().toObj() );
      }

      //check count
      if( actRecs.length !== expRecs.length )
      {
         throw "expect count: " + expRecs.length + ", actual count: " + actRecs.length + ", actual recs in cl:" + JSON.stringify( actRecs ) + ", expect recs:" + JSON.stringify( expRecs );
      }

   }
   catch( e )
   {
      throw new Error( e );
   }

   try
   {
      //check every records every fields
      for( var i in expRecs )
      {
         var actRec = actRecs[i];
         var expRec = expRecs[i];
         for( var f in expRec )
         {
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
               throw "error occurs in " + ( parseInt( i ) + 1 ) + "th record, in field' " + f + "'expect record: " + JSON.stringify( expRec ) + ", actual record: " + JSON.stringify( actRec );
            }
         }
      }

   }
   catch( e )
   {
      throw new Error( e );
   }

   try
   {
      //check every records every fields, actRecs as compare source
      for( var i in actRecs )
      {
         var actRec = actRecs[i];
         var expRec = expRecs[i];

         for( var f in actRec )
         {
            if( f == "_id" )
            {
               continue;
            }
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
               throw "error occurs in " + ( parseInt( i ) + 1 ) + "th record, in field' " + f + "'expect record: " + JSON.stringify( expRec ) + ", actual record: " + JSON.stringify( actRec );
            }
         }
      }

   }
   catch( e )
   {
      throw new Error( e );
   }
}

/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function insertData ( dbcl, number )
{
   try
   {
      if( undefined == this.number )
      {
         this.number = 1000;
      }
      var docs = [];
      for( var i = 0; i < number; ++i )
      {
         var no = i;
         var a = i;
         var user = "test" + i;
         var phone = 13700000000 + i;
         var time = new Date().getTime();
         var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
         //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

         docs.push( doc );
      }
      dbcl.insert( docs );

   }
   catch( e )
   {
      throw new Error( e );
   }

}

/************************************
*@Description: check the new cl name
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkRenameCLResult ( csName, oldCLName, newCLName )
{
   //check the old cl is not exist
   try
   {
      var clFullName = csName + "." + newCLName;
      var getNewCLName = db.snapshot( SDB_SNAP_COLLECTIONS, { "Name": clFullName } ).current().toObj().Name;
      if( getNewCLName !== clFullName )
      {
         throw "check the new cl name, old cl name: " + clFullName + ", new cl name: " + getNewCLName;
      }
      db.getCS( csName ).getCL( oldCLName );
      throw "need throw error";
   }
   catch( e )
   {
      if( e !== -23 )
      {
         throw new Error( e );
      }
   }
}

/************************************
*@Description: check the new cs name
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkRenameCSResult ( oldCSName, newCSName, clNum )
{
   try
   {
      var newCSObj = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { "Name": newCSName } ).current().toObj();
      var getNewCSName = newCSObj.Name;
      if( getNewCSName !== newCSName )
      {
         throw "check the new cs name, expect cs name: " + newCSName + ", actual cs name: " + getNewCSName;
      }

      var clArray = newCSObj.Collection;

      if( clNum != clArray.length )
      {
         throw "check cl num, expect cl num: " + clNum + ", actual cl num: " + clArray.length;
      }

      for( i = 0; i < clArray.length; i++ )
      {
         var csname = clArray[i].Name.split( "." )[0];
         if( csname !== newCSName )
         {
            throw "expect cs name:" + newCSName + ", actual cs name: " + csname;
         }
      }

      //check the old cl is not exist
      db.getCS( oldCSName );
      throw new Error( "CS_IS_EXIT" );
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw new Error( e );
      }
   }
}

/************************************
*@Description: 校验记录数
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkCount ( dbcl, expRecordNums, options )
{
   if( options == undefined )
   {
      options = null;
   }
   try
   {
      var count = dbcl.count( options );
      if( Number( count ) !== Number( expRecordNums ) )
      {
         throw "expect record num: " + expRecordNums + ", actual record num: " + count;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

}

/************************************
*@Description: 事务中renameCL
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function renameCLInTrans ( dbcs, clName, newCLName )
{
   try
   {
      dbcs.renameCL( clName, newCLName );
      throw "rename cl in trans should be fail!";
   }
   catch( e )
   {
      if( -336 !== e )
      {
         throw new Error( e );
      }
   }
}

/************************************
*@Description: coord1未提交事务，coord2执行renameCL
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function renameCLExistTrans ( db, csName, clName, newCLName )
{
   try
   {
      var dbcs = db.getCS( csName );
      dbcs.renameCL( clName, newCLName );
      throw "rename cl should be fail!";
   }
   catch( e )
   {
      if( -190 !== e )
      {
         throw new Error( e );
      }
   }
}
