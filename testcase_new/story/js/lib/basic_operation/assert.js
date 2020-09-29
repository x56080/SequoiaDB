
function Assert ()
{

   this.equal =
      function( actual, expected, message )
      {
         if( arguments.length < 2 ) { throw new Error( "actual and expected number less than 2" ); }
         message = message || "";

         if( !commCompareObject( actual, expected ) )
         {
            throw new Error( "equal error\nactual:" + JSON.stringify( actual, "", 1 ) + "\nexpected:" + JSON.stringify( expected, "", 1 ) + "\n" + message );
         }
      }

   this.notEqual =
      function( actual, expected, message )
      {
         if( arguments.length < 2 ) { throw new Error( "actual and expected number less than 2" ); }
         message = message || "";

         if( commCompareObject( actual, expected ) )
         {
            throw new Error( "notEqual error\nactual:" + JSON.stringify( actual, "", 1 ) + "\nexpected:" + JSON.stringify( expected, "", 1 ) + "\n" + message );
         }
      }

   /******************************************************************************
   @description    期望失败实际却执行成功
   @author  lyy
   @parameter
      errno         {array|string|number}    :     必填项，错误码
      func          {function}               :     必填项，执行函数名，不带括号！
      执行函数参数   {any}                    :     选填项，需要为执行函数传递的参数
   @usage
      e.g:
         commTryThrow("-6",db.dropCS,1);
         commTryThrow( -23, db.getCS( "exist_cs" ).getCL, "not_exist_cl" );
   ******************************************************************************/
   this.tryThrow =
      function( errno, func )
      {
         if( !Array.isArray( errno ) ) { errno = [Number( errno )]; }
         commCheckType( func, "function" );

         try
         {
            func();
            throw new Error( "should error but success" );
         } catch( e )
         {
            var err = e.message || e;
            if( errno.indexOf( Number( err ) ) === -1 )
            {
               throw e;
            }
         }
      }
}