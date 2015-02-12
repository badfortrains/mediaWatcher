var Watcher = require('./build/Release/watcher').Watcher;
var EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}


inherits(Watcher, EventEmitter);

Watcher.prototype.openAndPlay = function(trackItem,callback){
  var self = this;
  this.openTrack(trackItem,function(resO){
    self.play(function(resP){
      callback({res: resP});
    });
  });
};

/**
 * @param {UUID} uuid of renderer
 * @param {Interger Milliseconds} position to seek to
 */
Watcher.prototype.setPosition = function(uuid,position){
  var hours = Math.floor(position / 3600000),
      minutes,
      seconds,
      target;

  position -= hours * 3600000;
  minutes = Math.floor(position / 60000);
  position -= minutes * 60000;
  seconds = Math.floor(position / 1000);

  hours = (hours < 10) ? "0"+hours : hours;
  minutes = (minutes < 10) ? "0"+minutes : minutes;
  seconds = (seconds < 10) ? "0"+seconds : seconds;

  target = hours + ":" + minutes + ":" + seconds;
  this.setRenderer(this.uuid);
  this.seek(target);
}

Watcher.prototype.getTrackPosition = function(uuid){
  var self = this;
  this.setRenderer(uuid);
  this.getPosition(function(err,result){
    if(!err){
      result.uuid = uuid;
      self.emit("gotPosition",result);
    }
  })
}



var mw = new Watcher();

module.exports = mw;