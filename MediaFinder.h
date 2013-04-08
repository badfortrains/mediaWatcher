/*****************************************************************
|
|   Platinum - Micro Media Controller
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

#ifndef _MEDIA_FINDER_H_
#define _MEDIA_FINDER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Platinum.h"
#include "PltMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "PltMediaController.h"
#include "NptMap.h"
#include "NptStack.h"
#include "NptArray.h"

/*----------------------------------------------------------------------
 |   definitions
 +---------------------------------------------------------------------*/
typedef NPT_Map<NPT_String, NPT_String>        PLT_StringMap;
typedef NPT_Lock<PLT_StringMap>                PLT_LockStringMap;
typedef NPT_Map<NPT_String, NPT_String>::Entry PLT_StringMapEntry;

enum EventSource {SERVER , RENDERER};

struct EventInfo{
	NPT_String UUID;
	NPT_String Name;
	NPT_String Value;
    void* userData;
    EventSource SourceType;
};

struct ServerInfo{
    NPT_String iconUrl;
    NPT_String baseUrl;
    int test;
};

struct Info_data{
    NPT_SharedVariable shared_var;
    NPT_Result         res;
    PLT_MediaInfo   info;
};

struct Position_data{
    NPT_SharedVariable shared_var;
    NPT_Result         res;
    PLT_PositionInfo   info;
};

struct controllerInfo{
	NPT_Mutex m_ChangedLock;
	NPT_SharedVariable hasChanged;
	NPT_Lock< NPT_Stack<NPT_String> > m_DeviceStack;
	NPT_Lock< NPT_Stack<EventInfo> >m_EventStack;
};


/*----------------------------------------------------------------------
 |   Media_Finder
 +---------------------------------------------------------------------*/
class Media_Finder : public PLT_SyncMediaBrowser,
                                 public PLT_MediaController,
                                 public PLT_MediaControllerDelegate
{
public:
    Media_Finder(PLT_CtrlPointReference& ctrlPoint, controllerInfo* cB);
    virtual ~Media_Finder();

    // PLT_MediaBrowserDelegate methods
    bool OnMSAdded(PLT_DeviceDataReference& device);
    // PLT_MediaBrowserDelegate methods
    void OnMSRemoved(PLT_DeviceDataReference& device);

    // PLT_MediaControllerDelegate methods
    bool OnMRAdded(PLT_DeviceDataReference& device);
    void OnMRRemoved(PLT_DeviceDataReference& device);
    void OnMRStateVariablesChanged(PLT_Service*  service , 
                                   NPT_List<PLT_StateVariable*>*  vars );
    void OnMSStateVariablesChanged(PLT_Service*  service , 
                                   NPT_List<PLT_StateVariable*>*  vars );
    void OnSetAVTransportURIResult(NPT_Result               res, 
                                     PLT_DeviceDataReference& device,  
                                     void*                    userdata)
		{OnCommandResult(res,device,userdata);};
    void OnSetNextAVTransportURIResult(NPT_Result               res, 
                                     PLT_DeviceDataReference& device,  
                                     void*                    userdata)
		{OnCommandResult(res,device,userdata);};
    void OnStopResult(NPT_Result               res, 
                                     PLT_DeviceDataReference& device,  
                                     void*                    userdata)
		{OnCommandResult(res,device,userdata);};
    void OnPlayResult(NPT_Result               res, 
	                             PLT_DeviceDataReference& device,  
	                             void*                    userdata)
	         {OnCommandResult(res,device,userdata);};
    void OnPauseResult(NPT_Result               res, 
	                             PLT_DeviceDataReference& device,  
	                             void*                    userdata)
	         {OnCommandResult(res,device,userdata);};
    void OnSetMuteResult(NPT_Result               res, 
	                             PLT_DeviceDataReference& device,  
	                             void*                    userdata)
	         {OnCommandResult(res,device,userdata);};
    void OnSetVolumeResult(NPT_Result               res, 
	                             PLT_DeviceDataReference& device,  
	                             void*                    userdata)
	         {OnCommandResult(res,device,userdata);};
    void OnSeekResult(
        NPT_Result                  res,
        PLT_DeviceDataReference&    device,
        void*                       userdata) 
    {
        OnCommandResult(res,device,userdata);
    }

    void OnGetMediaInfoResult(
        NPT_Result                  res, 
	    PLT_DeviceDataReference&    device,
		PLT_MediaInfo*			    info,
	    void*                       userdata);

    void OnGetPositionInfoResult(
        NPT_Result                  res,
        PLT_DeviceDataReference&    device,
        PLT_PositionInfo*           info,
        void*                       userdata);

    NPT_Result  StopTrack(PLT_BrowseData* status);
	NPT_Result  PlayTrack(PLT_BrowseData* status);
    NPT_Result  PauseTrack(PLT_BrowseData* status);
	NPT_Result  Mute(bool value, PLT_BrowseData* status);
    NPT_Result  Volume(int value, PLT_BrowseData* status);
    NPT_Result  GetTrackInfo(Info_data* status);
    NPT_Result  GetPosition(Position_data* status);
    NPT_Result  SetPosition(PLT_BrowseData* status, NPT_String target);
	NPT_Result  DoSearch(NPT_String UUID,const char* object_id,PLT_MediaObjectListReference& resultList);
	NPT_Result  OpenTrack(NPT_Array<PLT_MediaItemResource> Resources,NPT_String Didl, PLT_BrowseData* status);
    NPT_Result  DoBrowse(NPT_String UUID,const char* object_id, PLT_MediaObjectListReference& resultList);
	NPT_Result  OpenNextTrack(NPT_Array<PLT_MediaItemResource> Resources,NPT_String Didl, PLT_BrowseData* status);
	NPT_Result	SetMR(NPT_String UUID);
	PLT_DeviceMap GetMRs();


private: 
	PLT_DeviceDataReference getDeviceReference(NPT_String UUID);
	void		GetCurMR(PLT_DeviceDataReference& renderer);
 	void OnCommandResult(NPT_Result               res, 
                                     PLT_DeviceDataReference& device,  
                                     void*                    userdata);

private:
	controllerInfo* cBaton;
    /* The tables of known devices on the network.  These are updated via the
     * OnMSAddedRemoved and OnMRAddedRemoved callbacks.  Note that you should first lock
     * before accessing them using the NPT_Map::Lock function.
     */
    NPT_Lock<PLT_DeviceMap> m_MediaServers;
    NPT_Lock<PLT_DeviceMap> m_MediaRenderers;

    /* The currently selected media server as well as 
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaServer;
    NPT_Mutex               m_CurMediaServerLock;

    /* The currently selected media renderer as well as 
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaRenderer;
    NPT_Mutex               m_CurMediaRendererLock;

    /* the most recent results from a browse request.  The results come back in a 
     * callback instead of being returned to the calling function, so this 
     * global variable is necessary in order to give the results back to the calling 
     * function.
     */
    PLT_MediaObjectListReference m_MostRecentBrowseResults;

    /* When browsing through the tree on a media server, this is the stack 
     * symbolizing the current position in the tree.  The contents of the 
     * stack are the object ID's of the nodes.  Note that the object id: "0" should
     * always be at the bottom of the stack.
     */
    NPT_Stack<NPT_String> m_CurBrowseDirectoryStack;



    /* the semaphore on which to block when waiting for a response from over
     * the network 
     */
    NPT_SharedVariable m_CallbackResponseSemaphore;
};

#endif /* _MICRO_MEDIA_CONTROLLER_H_ */

