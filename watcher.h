#ifndef NODE_WATCHER_H
#define NODE_WATCHER_H

#include <nan.h>

#include <v8.h>
#include <node.h>

#include "MediaController.h"

using namespace node;
using namespace v8;


#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    Local<Function> var;                                                       \
    if (args.Length() > i && !args[i]->IsUndefined()) {                        \
        if (!args[i]->IsFunction()) {                                          \
            return NanThrowTypeError("Argument " #i " must be a function");    \
        }                                                                      \
        var = Local<Function>::Cast(args[i]);                                  \
    }

#define OPTIONAL_CB_ACTION(i, var)												\
    OPTIONAL_ARGUMENT_FUNCTION(i, callbackHandle);							 	\
    NanCallback *callback = new NanCallback(callbackHandle);					\
    var = new CBAction(callback);	

class Watcher : public ObjectWrap{
public:
	static Persistent<FunctionTemplate> constructor_template;
	static uv_async_t async;
	static PLT_UPnP upnp;
    static PLT_CtrlPointReference ctrlPoint;
    static void Init(Handle<Object> target);
    static void AsyncCB(uv_async_t *handle, int status /*UNUSED*/);


protected:
    MediaController* mc;

	

	Watcher() : ObjectWrap() {};
	~Watcher(){};


	static NAN_METHOD(New);
	static NAN_METHOD(Browse);
	static NAN_METHOD(GetTracks);
	static NAN_METHOD(GetTrackPosition);
	static NAN_METHOD(SetRenderer);
	static NAN_METHOD(OpenTrack);
	static NAN_METHOD(OpenNextTrack);
	static NAN_METHOD(Play);
	static NAN_METHOD(Stop);
	static NAN_METHOD(Pause);
	static NAN_METHOD(Next);

};

#endif