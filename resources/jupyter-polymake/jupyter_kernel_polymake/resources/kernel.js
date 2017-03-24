/** FIXME this is the only js file loaded, so the code redefining the completer regex must go here, 
          but it doesn't really look nice with the minified code below

          Also, wihtout the onload function, jupyter complains that there is no asynchronous module definition found in this file,
          maybe threejs should be loaded differently
*/
define(function(){

    var onload = function(){
		var completer = require("notebook/js/completer");
		completer.Completer.reinvoke_re=/[%><0-9a-z._/\\:~-]/i;
        //console.log(completer.Completer.reinvoke_re);
    }

    return {onload:onload}
})
