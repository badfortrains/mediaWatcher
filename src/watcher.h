#ifndef NODE_WATCHER_H
#define NODE_WATCHER_H

#include <nan.h>

#include <v8.h>
#include <node.h>

#include "mediaController.h"

using namespace node;
using namespace v8;

#define REQUIRE_ARGUMENT_INTEGER(i, var)                                        \
    if (info.Length() <= (i) || !info[i]->IsInt32()) {                        	\
        return Nan::ThrowTypeError("Argument " #i " must be an integer");        	\
    }                                                                          	\
    int var(info[i]->IntegerValue());

#define REQUIRE_ARGUMENT_STRING(i, var)                                        \
    if (info.Length() <= (i) || !info[i]->IsString()) {                        \
        return Nan::ThrowTypeError("Argument " #i " must be a string");          \
    }                                                                          \
    String::Utf8Value var(info[i]->ToString());

#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    Local<Function> var;                                                       \
    if (info.Length() > i && !info[i]->IsUndefined()) {                        \
        if (!info[i]->IsFunction()) {                                          \
            return Nan::ThrowTypeError("Argument " #i " must be a function");  \
        }                                                                      \
        var = Local<Function>::Cast(info[i]);                                  \
    }

#define OPTIONAL_CB_ACTION(i, var)											   \
    Local<Function> callbackHandle;                                            \
    if (info.Length() > i && !info[i]->IsUndefined()) {                        \
        if (!info[i]->IsFunction()) {                                          \
            return Nan::ThrowTypeError("Argument " #i " must be a function");  \
        }                                                                      \
        Nan::Callback *callback = new Nan::Callback(info[i].As<Function>());   \
        var = new CBAction(callback);                                          \
    }


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
    static Nan::Persistent<v8::Function> constructor;

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
	static NAN_METHOD(SetVolume);
	static NAN_METHOD(Seek);
	static NAN_METHOD(GetRenderers);
    static NAN_METHOD(GetServers);

};

#endif
