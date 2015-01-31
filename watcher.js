var Watcher = require("./build/Debug/watcher.node").Watcher;
var EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}


console.log(Watcher)
inherits(Watcher, EventEmitter);
var mw = new Watcher();

mw.on("serverAdded",function(ev){
	console.log("serverAdded",ev)
	mw.browse(ev.uuid,"0",function(){
		console.log("got results")
		console.log(arguments)
	})
})

mw.on("rendererAdded",function(ev){
	console.log("rendererAdded",ev)
})