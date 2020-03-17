/* *****************************************************************************
@discription: return new Ssh
@author: zhaoxiaoni
@parameter
   hostname:String
   user:string
   password:string
   port:int
@e.g.
   var ssh = new SshObj( hostname, user, password, port );
***************************************************************************** */
function SshObj( hostname, user, password, port )
{
   if( password === undefined ) { password = ""; }
   if( port === undefined ) { port = 22; }
   try
   {
      var ssh = new Ssh( hostname, user, password, port );
   }
   catch( e )
   {
      println( getLastErrMsg() );
      throw new Error( e );
   }

   this.close =
      function()
      {
         try
         {
            ssh.close();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.exec =
      function( command )
      {
         try
         {
            var result = ssh.exec( command );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return result;
      }

   this.getLastOut = 
      function()
      {
         try
         {
            var result = ssh.getLastOut();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return result;
      }

   this.getLastRet = 
      function()
      {
         try
         {
            var result = ssh.getLastRet();
         }
         catch( e )
         {
            throw new Error( e );
         }  
         return result;
      }
   
   this.getLocalIP =
      function()
      {
         try
         {
            var result = ssh.getLocalIP();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return result;
      }

   this.getPeerIP =
      function()
      {
         try
         {
            var result = ssh.getPeerIP();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return result;
      }

   this.pull =
      function( local_file, dst_file, mode )
      {
         if( mode === undefined ){ mode = 0640; }
         try
         {
            ssh.pull( local_file, dst_file, mode );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.push =
      function( local_file, dst_file, mode )
      {
         if( mode === undefined ){ mode = 0755; }
         try
         {
            ssh.push( local_file, dst_file, mode );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }
}
