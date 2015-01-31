#include "Actions.h"
#include <nan.h>
using namespace v8;

DeviceAction::DeviceAction(NPT_String eventName, PLT_DeviceDataReference& device) :
    eventName(eventName)
{
    deviceName = device->GetFriendlyName();
    uuid = device->GetUUID();
    baseUrl = device->GetIconUrl();
    iconUrl = device->GetURLBase().ToString();
}

void 
DeviceAction::EmitAction(ObjectWrap* context){
    Local<Object> event = Object::New();
    event->Set(NanNew("uuid"),NanNew<String>(uuid));
    event->Set(NanNew("baseUrl"),NanNew<String>(baseUrl));
    event->Set(NanNew("iconUrl"),NanNew<String>(iconUrl));
    event->Set(NanNew("name"),NanNew<String>(deviceName));

    Local<Value> args[] = { NanNew<String>(eventName),event };
    EMIT_EVENT(NanObjectWrapHandle(context), 2, args);
}