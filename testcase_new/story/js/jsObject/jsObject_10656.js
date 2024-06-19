/******************************************************************************
*@Description: seqDB-10656:System对象枚举已登录用户
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].listLoginUsers();
   }
}

SystemTest.prototype.listLoginUsers = function()
{
   this.init();

   var users = this.system.listLoginUsers( { detail: true } ).toArray();
   var info = this.cmd.run( "who | sed 's/  */ /g'" ).split( "\n" );
   /// remove empty line
   for ( var i = 0 ; i < info.length ; )
   {
      if ( 0 == info[i].length )
      {
         info.splice( i, 1 ) ;
      }
      else
      {
         ++i ;
      }
   }

   for( var i = 0; i < users.length; i++ )
   {
      var userObj = JSON.parse( users[i] );
      var tmp = info[i].split( " " );
      var len = tmp.length;
      var username = tmp[0];             // 用户名
      var tty = tmp[1];                  // 登录终端
      var time = tmp[2];                 // 登录时间
      var addr = "";                     // 登录的主机名或者ip
      var fromEndPos = - 1 ;
      var fromBeginPos = -1 ;

      for( var j = 3; j < len; j++ )
      {
         var strlen = tmp[j].length ;
         if ( strlen == 0 )
         {
            continue ;
         }
         if( tmp[j][0] == "(" )
         {
            fromBeginPos = j ;
            if ( tmp[j][strlen-1] == ")" )
            {
               fromEndPos = j ;
               addr = tmp[j].slice( 1, strlen - 1 ) ;
            }
            else
            {
               addr = tmp[j].slice( 1, strlen ) ;
			}
            break;
         }
         time += " " + tmp[j];
      }

      if ( fromBeginPos != -1 && fromEndPos == -1 )
      {
         for ( var j = fromBeginPos + 1 ; j < len ; j++ )
         {
            var strlen = tmp[j].length ;
            if ( 0 == strlen )
            {
               continue ;
            }
            addr += " " ;
            if ( tmp[j][strlen-1] == ")" )
            {
               fromBeginPos = j ;
               addr += tmp[j].slice( 0, strlen - 1 ) ;
               break ;
            }
            else
            {
               addr += tmp[j] ;
			}
         }
      }

      if( username !== userObj.user || tty !== userObj.tty || time !== userObj.time || addr !== userObj.from )
      {
         var parsedStr = "{ User: " + username + ", tty: " + tty + ", time: " + time + ", addr: " + addr + " }" ;
         throw new Error( "userObj: " + JSON.stringify( userObj ) + ", ParsedInfo: " + parsedStr + ", SourceLine: " + info[i] );
      }
   }

   this.release();
}

main( test );
