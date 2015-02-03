#ifndef NODE_ACTIONS_H
#define NODE_ACTIONS_H

#include "Neptune.h"
#include <queue>
#include "Platinum.h"
#include <node.h>
#include <nan.h>

using namespace node;

#define TRY_CATCH_CALL(context, callback, argc, argv)                          \
    NanMakeCallback((context), (callback), (argc), (argv))

#define EMIT_EVENT(obj, argc, argv)                                            \
    TRY_CATCH_CALL((obj),                                                      \
        Local<Function>::Cast((obj)->Get(NanNew("emit"))),                     \
        argc, argv                                                             \
    );



class Action{
public:
    virtual void EmitAction(ObjectWrap *context) = 0;
    virtual ~Action(){};

};

class CBAction : public Action{
public:
    CBAction(NanCallback *callback) : callback(callback){};
    virtual ~CBAction();

    void EmitAction(ObjectWrap *context);
    void SetError(NPT_Result err);

    virtual void ErrorCB(NPT_Result err);
    virtual void SuccessCB() = 0;



protected:
    NanCallback *callback;
    NPT_Result res;
};

class GetTrackPositionAction : public CBAction{
public:
    GetTrackPositionAction(NanCallback *callback) : CBAction(callback){};
    virtual ~GetTrackPositionAction(){};
    void GotResult(PLT_PositionInfo positionInfo);
    void SuccessCB();

private:
    PLT_PositionInfo info;
};

class DeviceAction : public Action{
public:
    DeviceAction(NPT_String eventName, PLT_DeviceDataReference& device);
    virtual ~DeviceAction(){};
    void EmitAction(ObjectWrap *context);
    NPT_String uuid;
    NPT_String iconUrl;
    NPT_String baseUrl;
    NPT_String deviceName;
    NPT_String eventName;
};

#endif