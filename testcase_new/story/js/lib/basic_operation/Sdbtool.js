var tmpSdbtool = {
   help: Sdbtool.prototype.help
};
var funcSdbtool = Sdbtool;
var funcSdbtoolhelp = Sdbtool.help;
var funcSdbtoollistNodes = Sdbtool.listNodes;
Sdbtool=function(){try{return funcSdbtool.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
Sdbtool.help = function(){try{ return funcSdbtoolhelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
Sdbtool.listNodes = function(){try{ return funcSdbtoollistNodes.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
Sdbtool.prototype.help=function(){try{return tmpSdbtool.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
