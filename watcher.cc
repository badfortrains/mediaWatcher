#include <node.h>
#include <uv.h>

#include "watcher.h"


void Watcher::Init(Handle<Object> target){
	NanScope();

	Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(NanNew("Watcher"));

    NanAssignPersistent(constructor_template, t);

    //NPT_LogManager::GetDefault().Configure("plist:.level=INFO;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=24");
    Watcher::ctrlPoint = new PLT_CtrlPoint();
    Watcher::upnp.AddCtrlPoint(Watcher::ctrlPoint);
    Watcher::upnp.Start();

    NODE_SET_PROTOTYPE_METHOD(t, "browse", Browse);

    target->Set(NanNew("Watcher"),
        t->GetFunction());
}

void Watcher::AsyncCB(uv_async_t *handle, int status /*UNUSED*/){
	Watcher* watcher = (Watcher*) handle->data;
    watcher->mc->FlushQueue(watcher);
}

NAN_METHOD(Watcher::New) {
    NanScope();

    if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use the new operator to create new Watcher objects");
    }

    Watcher* mw = new Watcher();
    mw->Wrap(args.This());

    uv_async_init(uv_default_loop(),&(Watcher::async),Watcher::AsyncCB);
    Watcher::async.data = (void*) mw;

    mw->mc = new MediaController(Watcher::ctrlPoint,&Watcher::async);

    NanReturnValue(args.This());
}

//uuid, dir, callback
NAN_METHOD(Watcher::Browse){
    NanScope();

    if (args.Length() < 3) {
        return NanThrowTypeError("Expected 3 arguments");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.This());
    Local<Function> callbackHandle = args[2].As<Function>();
    NanCallback *callback = new NanCallback(callbackHandle);

    watcher->mc->BrowseDirectory(callback,*String::Utf8Value(args[0]),*String::Utf8Value(args[1]));
    NanReturnUndefined();
}



Persistent<FunctionTemplate> Watcher::constructor_template;
uv_async_t Watcher::async;
PLT_UPnP Watcher::upnp;
PLT_CtrlPointReference Watcher::ctrlPoint;


extern "C" {
  static void init (Handle<Object> target)
  {
    Watcher::Init(target);
  }

  NODE_MODULE(watcher, init);
}