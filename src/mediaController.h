#ifndef NODE_MEDIACONTROLLER_H
#define NODE_MEDIACONTROLLER_H

#include <node.h>
#include <uv.h>

#include "Neptune.h"
#include <queue>
#include "Platinum.h"
#include "PltMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "PltMediaController.h"
#include "NptMap.h"
#include "NptStack.h"

#include <nan.h>
#include "actions.h"

using namespace node;
using namespace v8;



class MediaController : public PLT_SyncMediaBrowser,
                        public PLT_MediaController,
               			public PLT_MediaControllerDelegate{

public:
	MediaController(PLT_CtrlPointReference& ctrlPoint, uv_async_t* async);
	virtual ~MediaController(){};
    // PLT_MediaBrowserDelegate methods
    bool OnMSAdded(PLT_DeviceDataReference& device);
    bool OnMRAdded(PLT_DeviceDataReference& device);
    void OnMRRemoved(PLT_DeviceDataReference& device);
    void OnMSRemoved(PLT_DeviceDataReference& device);

    void OnMRStateVariablesChanged(PLT_Service*  service, NPT_List<PLT_StateVariable*>*  vars);
    void OnMSStateVariablesChanged(PLT_Service*  service, NPT_List<PLT_StateVariable*>*  vars);

    void OnCBActionResult(NPT_Result res, void* action);
    void OnNextResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnStopResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnPauseResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnSetVolumeResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnSeekResult(NPT_Result res, PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };
    void OnSetAVTransportURIResult(NPT_Result res,PLT_DeviceDataReference& /* device */,void* action){ OnCBActionResult(res,action); };

    void OnGetPositionInfoResult(NPT_Result res, PLT_DeviceDataReference& /* device */,PLT_PositionInfo* info,void* action);


    //Only use on main chrome thread!
    void FlushQueue(ObjectWrap *context);

    void BrowseDirectory(NanCallback *callback, NPT_String uuid,NPT_String objectId);
    void GetTracks(NanCallback *callback, NPT_String uuid,NPT_String objectId);
    NPT_Result OpenTrack(NPT_Array<PLT_MediaItemResource> &Resources,NPT_String& Didl,Action* action);
    NPT_Result OpenNextTrack(NPT_Array<PLT_MediaItemResource> &Resources,NPT_String& Didl,Action* action);
    NPT_Result GetTrackPosition(Action* action);
    NPT_Result SetRenderer(NPT_String uuid);

    void GetCurMR(PLT_DeviceDataReference& renderer);
    PLT_DeviceMap GetMRs();
    PLT_DeviceMap GetMSs();
protected:
    std::queue<Action*> queue;
    uv_async_t* async;


    PLT_DeviceDataReference getDeviceReference(NPT_String UUID);

private:
    /* Tables of known devices on the network.  These are updated via the
     * OnMSAddedRemoved and OnMRAddedRemoved callbacks.  Note that you should first lock
     * before accessing them using the NPT_Map::Lock function.
     */
    NPT_Lock<PLT_DeviceMap> m_MediaServers;
    NPT_Lock<PLT_DeviceMap> m_MediaRenderers;

    /* Currently selected media server as well as
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaServer;
    NPT_Mutex               m_CurMediaServerLock;

    /* Currently selected media renderer as well as
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaRenderer;
    NPT_Mutex               m_CurMediaRendererLock;
};


#endif
