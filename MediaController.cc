#include <uv.h>
#include "MediaController.h"

MediaController::MediaController(PLT_CtrlPointReference& ctrlPoint, uv_async_t* async) :
    PLT_SyncMediaBrowser(ctrlPoint),
    PLT_MediaController(ctrlPoint),
    async(async)
{
    PLT_MediaController::SetDelegate(this);
}


bool 
MediaController::OnMSAdded(PLT_DeviceDataReference& device) 
{   
    NPT_String uuid = device->GetUUID();

    // Issue special action upon discovering MediaConnect server
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:*", service))) {
        PLT_ActionReference action;
        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsAuthorized", 
            action);
        if (!action.IsNull()) {
            action->SetArgumentValue("DeviceID", "");
            PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);
        }

        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsValidated", 
            action);
        if (!action.IsNull()) {
            action->SetArgumentValue("DeviceID", "");
            PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);
        }
    }

    DeviceAction* event = new DeviceAction("serverAdded",device);
    queue.push(event);
    uv_async_send(async);

    PLT_DeviceDataReference* device2;
    (this)->GetMediaServersMap().Get(uuid, device2);

    return true; 
}

bool 
MediaController::OnMRAdded(PLT_DeviceDataReference& device) 
{   
    NPT_String uuid = device->GetUUID();
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:*", service))) {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Put(uuid, device);

        DeviceAction* event = new DeviceAction("rendererAdded",device);
        queue.push(event);
        uv_async_send(async);
    }

    return true; 
}

class BrowseAction : public NanAsyncWorker {
public:
    BrowseAction(NanCallback *callback, NPT_String uuid,NPT_String objectId, MediaController* mc)
    : NanAsyncWorker(callback), uuid(uuid), objectId(objectId), mc(mc) {}
    ~BrowseAction() {}

    void Execute () {
        res = NPT_FAILURE;
        PLT_DeviceDataReference* device = NULL;
        mc->GetMediaServersMap().Get(uuid, device);
        if (device) {
            // send off the browse packet and block
            res = mc->BrowseSync(
                *device,
                objectId,
                resultList,
                false);
        }
    }

    void HandleOKCallback(){
        NanScope();
        if(NPT_SUCCEEDED(res)){
            Local<Array> dirArray = Array::New();
            if(!resultList.IsNull()){
                NPT_List<PLT_MediaObject*>::Iterator item = resultList->GetFirstItem();
                int i =0;
                while (item) {
                    Local<Object> temp = Object::New();
                    temp->Set(NanNew("_id"),NanNew<String>((*item)->m_ObjectID));
                    temp->Set(NanNew("title"),NanNew<String>((*item)->m_Title));
                    temp->Set(NanNew("isContainer"),NanNew<Boolean>((*item)->IsContainer()));
                    dirArray->Set(i++,temp);
                    ++item;
                }
            }
            Local<Value> argv[] = {
                NanNull()
              , dirArray
            };
            callback->Call(2, argv);
        }else{
            Local<Value> argv[] = {
                NanError("Failed to retrieve directory"),
                NanNew<String>(uuid),
                NanNew<String>(objectId)
            };
            callback->Call(3, argv);
        }
    }
private:
    NPT_Result res;
    NPT_String uuid;
    NPT_String objectId;
    PLT_MediaObjectListReference resultList;
    MediaController* mc;
};

void 
MediaController::BrowseDirectory(NanCallback *callback, NPT_String uuid,NPT_String objectId){
    NanAsyncQueueWorker(new BrowseAction(callback, uuid, objectId,this));
}

PLT_DeviceDataReference
MediaController::getDeviceReference(NPT_String UUID){

    NPT_AutoLock             lock(m_MediaServers);
    PLT_DeviceDataReference* result = NULL;
    m_MediaServers.Get(UUID, result);
    return result?*result:PLT_DeviceDataReference();
}

void
MediaController::FlushQueue(ObjectWrap *context){
    while (!queue.empty()) {
        Action* event = queue.front();
        event->EmitAction(context);
        queue.pop();
        delete event;
    }
}