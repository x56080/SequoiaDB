/******************************************************************************
*@Description: seqDB-10649:System对象获取当前用户环境信息
*@author: Zhao Xiaoni
******************************************************************************/
function test()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].getUserEnv();
   }
}

SystemTest.prototype.getUserEnv = function()
{
   this.init();

   var tmpObj = {};
   var keys = [];
   var values = [];
   var environ = this.system.getUserEnv().toObj();
   var tmpInfo = this.cmd.run( "env" ).split( "\n" );
   for( var i = 0; i < tmpInfo.length - 1; i++ )
   {
      var index = tmpInfo[i].indexOf( "=" );
      keys[i] = tmpInfo[i].slice( 0, index );
      values[i] = tmpInfo[i].slice( index + 1 );
      tmpObj[ keys[i] ] = values[i];
   }

   for( var j in tmpObj )
   {
      if( tmpObj[j] !== environ[j] )
      {
         throw new Error( "tmpObj[" + j + "]: " + tmpObj[j] + ", environ[" + j + "]: " + environ[j] );
      }
   }

   this.release();
}

main( test );
