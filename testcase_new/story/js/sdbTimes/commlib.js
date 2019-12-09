/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2017-02-28 xiaoni huang
*******************************************************************************/
var cmd = cmdInit();

function cmdInit ()
{
   var cmd;
   try
   {
      cmd = new Cmd();
   }
   catch( e )
   {
      throw new Error( e );
   }
   return cmd;
}

function cmdRun ( str )
{
   var rc;
   try
   {
      rc = cmd.run( str ).split( "\n" )[0];
   }
   catch( e )
   {
      throw new Error( e );
   }
   return rc;
}

/* ****************************************************
@description: turn to local time
@parameter:
time: Timestamp with time zone to millisecond, eg:'1901-12-31T15:54:03.000Z'
format: eg:%Y-%m-%d-%H.%M.%S.000000
@return:
localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   var localtime;
   try
   {
      var msecond = new Date( time ).getTime();
      var second = parseInt( msecond / 1000 ); //millisecond to second
      localtime = cmdRun( 'date -d@"' + second + '" "+' + format + '"' );

   }
   catch( e )
   {
      throw new Error( e );
   }
   return localtime;
}

/*******************************************************************************
*@Description : 集合中不存在记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function checkCount ( cl, expRecordNum, findConf )
{
   if( typeof ( expRecordNum ) == "undefined" ) { expRecordNum = 0; };
   if( typeof ( findConf ) == "undefined" ) { findConf = null; };
   try
   {
      var cnt = cl.count( findConf );
      var actCnt = Number( cnt );
      if( expRecordNum !== actCnt )
      {
         throw "expect: " + expRecordNum + ", actual: " + actCnt;
      }

   }
   catch( e )
   {
      throw new Error( e );
   }

}

/*******************************************************************************
*@Description : 插入无效的记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function insertRecs ( cl, rawData )
{
   var i = 0;
   for( i = 0; i < rawData.length; i++ )
   {
      try
      {
         cl.insert( rawData[i] );
         throw "insert record: " + JSON.stringify( rawData[i] ) + "sucess, expect: failed.";
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw new Error( "insert record: " + JSON.stringify( rawData[i] ) + "failed, e:" + e );
         }
      }
   }

}

/*******************************************************************************
*@Description : 删除多条记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function removeDatas ( dbcl, array )
{
   var i = 0;
   try
   {
      for( i = 0; i < array.length; i++ )
      {
         dbcl.remove( array[i] );
      }

   }
   catch( e )
   {
      throw new Error( "remove record:" + JSON.Stringify( array[i] ) + "failed, e:" + e );
   }


}


/*******************************************************************************
@Description : 比较2个数组是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( actRecs, expRecs )
{
   try
   {
      //check count
      if( actRecs.length !== expRecs.length )
      {
         throw "expect count: " + expRecs.length + ", actual count: " + actRecs.length;
      }

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

}

/*******************************************************************************
@Description : 从数组中获取元素作为查询条件，将匹配的记录存入array中返回
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function cusorToArray ( cl, array )
{
   var i = 0;
   try
   {
      //get actual records to array
      var rcData = [];
      for( i = 0; i < array.length; i++ )
      {
         var cursor = cl.find( array[i], { _id: { $include: 0 } } ).sort( { a: 1 } );
         while( tmpRec = cursor.next() )
         {
            rcData.push( tmpRec.toObj() );
         }
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
   return rcData
}


/*******************************************************************************
*@Description : 更新多条记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function updateDatas ( dbcl, array )
{
   var i = 0;
   try
   {
      for( i = 0; i < array.length; i++ )
      {
         dbcl.update( { $set: { up: array[i]["b"] } }, array[i] );
      }

   }
   catch( e )
   {
      throw new Error( "update record:" + JSON.Stringify( array[i] ) + "failed, e:" + e );
   }
}


/*******************************************************************************
*@Description : 比对记录2个字段值是否相等
*@Modify list : 入参：游标
*              2019-11-19 zhaoyu
*******************************************************************************/
function checkUpdateRec ( rc )
{
   try
   {
      //get actual records to array
      var rcData = [];
      while( tmpRec = rc.next() )
      {
         //check result
         println( JSON.stringify( tmpRec.toObj() ) );
         var dt = JSON.stringify( tmpRec.toObj()["b"] ); //println( dt ); 
         var up = JSON.stringify( tmpRec.toObj()["up"] ); //println( up + "\n" ); 
         if( dt !== up )
         {
            throw "dt: " + dt + ", up: " + up;
         }
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

}
