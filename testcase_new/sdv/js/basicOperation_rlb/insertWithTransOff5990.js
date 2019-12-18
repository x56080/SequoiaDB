/*******************************************************************************
*@Description:   seqDB-5990:事务功能关闭，执行事务操作_SD.transaction.001
*@Author:        2019-4-18  wangkexin
********************************************************************************/
main();
function main ()
{
   var clName = "cl5990";
   if( commIsTransEnabled( db ) )
   {
      println( "transactionon is true." );
      return;
   }

   try
   {
      db.transBegin();
   }
   catch( e )
   {
      if( commIsStandalone( db ) )
      {
         //dpslocal配置参数默认为false,执行事务操作报-3错误，如dpslocal为true时，未开启事务进行事务操作会报-253，关于此参数的配置已手工验证
         if( e !== -253 && e !== -3 )
         {
            throw buildException( "main()", e, "unexpected error", '-253 or -3', e );
         }
         println( "run mode is standalone" );
         return;
      }
      else
      {
         throw e;
      }
   }

   try
   {
      var cl = commCreateCL( db, COMMCSNAME, clName );
      var rec = { "_id": 5990 };
      cl.insert( rec );
      throw "expected failure but actual success."
   }
   catch( e )
   {
      if( e !== -253 )
      {
         throw buildException( "main()", e, "unexpected error when exec insert operation", '-253', e );
      }
   }

   var count = cl.count();
   if( Number( count ) !== 0 )
   {
      throw "error number of count : " + Number( count );
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}

function commIsTransEnabled ( db )
{
   var isTrans = false;
   var COMMTMPCS = COMMCSNAME + "_tmp";
   var COMMTMPCL = COMMCLNAME + "_tmp";
   var tmpCL = commCreateCL( db, COMMTMPCS, COMMTMPCL, {}, true, true, "Judge transaction create collection" );
   try
   {
      db.transBegin();
      //tmpCL.update( {$unset:{a:1}}, {a:{$exists:0}} ) ; // do nothing
      tmpCL.remove();   // if transaction don't turn on, will throw error: -253
      isTrans = true;
      db.transCommit();
      commDropCS( db, COMMTMPCS, false, "drop CS in transation temp" )
   }
   catch( e )
   {
      commDropCS( db, COMMTMPCS, true, "drop CS in transation temp" )
      if( commCompareErrorCode( e, -253 ) )
      {
      }
      else
      {
         println( "execute transBegin happen error, e = " + e );
      }
   }
   // clear when all use cases finished
   return isTrans;
}