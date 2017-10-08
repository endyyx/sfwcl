
#ifdef _USE_ADVANCED_FEATURES

#include "Browser.h"
Browser::Browser(HWND _hWndParent)
{
	iComRefCount = 0;
	RECT rc;    
	GetWindowRect(_hWndParent,&rc);
	int width=rc.right - rc.left;
	int height = rc.bottom - rc.top;
	::SetRect(&rObject, 0,20,width,height);
	hWndParent = _hWndParent;  
	if (CreateBrowser() == FALSE){
		return;
	}  
	ShowWindow(GetControlWindow(), SW_SHOW);  
	this->Navigate(L"about:blank");
}  
bool Browser::CreateBrowser()
{
	HRESULT hr;
	hr = ::OleCreate(CLSID_WebBrowser,
					IID_IOleObject, OLERENDER_DRAW, 0, this, this,
					(void**)&oleObject);  
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Cannot create oleObject CLSID_WebBrowser"),
		_T("Error"),
		MB_ICONERROR);
		return FALSE;
	}
  
	hr = oleObject->SetClientSite(this);
	hr = OleSetContainedObject(oleObject, TRUE);
  
	RECT posRect;
	::SetRect(&posRect, 0, 0, 800, 600);
	hr = oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE,
							NULL, this, -1, hWndParent, &posRect);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("oleObject->DoVerb() failed"),
		_T("Error"),
		MB_ICONERROR);
		return FALSE;
	}
  
	hr = oleObject->QueryInterface(&webBrowser2);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("oleObject->QueryInterface(&webBrowser2) failed"),
		_T("Error"),
		MB_ICONERROR);
		return FALSE;
	}
  
	return TRUE;
}
  
RECT Browser::PixelToHiMetric(const RECT& _rc)
{
	static bool s_initialized = false;
	static int s_pixelsPerInchX, s_pixelsPerInchY;
	if(!s_initialized)
	{
		HDC hdc = ::GetDC(0);
		s_pixelsPerInchX = ::GetDeviceCaps(hdc, LOGPIXELSX);
		s_pixelsPerInchY = ::GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(0, hdc);
		s_initialized = true;
	}
  
	RECT rc;
	rc.left = MulDiv(2540, _rc.left, s_pixelsPerInchX);
	rc.top = MulDiv(2540, _rc.top, s_pixelsPerInchY);
	rc.right = MulDiv(2540, _rc.right, s_pixelsPerInchX);
	rc.bottom = MulDiv(2540, _rc.bottom, s_pixelsPerInchY);
	return rc;
}
  
void Browser::SetRect(const RECT& _rc)
{
	rObject = _rc;
	{
		RECT hiMetricRect = PixelToHiMetric(rObject);
		SIZEL sz;
		sz.cx = hiMetricRect.right - hiMetricRect.left;
		sz.cy = hiMetricRect.bottom - hiMetricRect.top;
		oleObject->SetExtent(DVASPECT_CONTENT, &sz);
	}
  
	if(oleInPlaceObject != 0)
	{
		oleInPlaceObject->SetObjectRects(&rObject, &rObject);
	}
}
  
// ----- Control methods -----
  
void Browser::GoBack(){
	this->webBrowser2->GoBack();
}
  
void Browser::GoForward(){
	this->webBrowser2->GoForward();
}
  
void Browser::Refresh(){
	this->webBrowser2->Refresh();
}
  
void Browser::Navigate(std::wstring szUrl){
	bstr_t url(szUrl.c_str());
	variant_t flags(0x02u); //navNoHistory
	this->webBrowser2->Navigate(url, &flags, 0, 0, 0);
}
  
// ----- IUnknown -----
  
HRESULT STDMETHODCALLTYPE Browser::QueryInterface(REFIID riid,void**ppvObject){
	if (riid == __uuidof(IUnknown))
	{
		(*ppvObject) = static_cast<IOleClientSite*>(this);
	}
	else if (riid == __uuidof(IOleInPlaceSite))
	{
		(*ppvObject) = static_cast<IOleInPlaceSite*>(this);
	}
	else
	{
		return E_NOINTERFACE;
	}
  
	AddRef();
	return S_OK;
}
  
ULONG STDMETHODCALLTYPE Browser::AddRef(void){
	iComRefCount++; 
	return iComRefCount;
}
  
ULONG STDMETHODCALLTYPE Browser::Release(void){
	iComRefCount--; 
	return iComRefCount;
}
  
// ---------- IOleWindow ----------
  
HRESULT STDMETHODCALLTYPE Browser::GetWindow(__RPC__deref_out_opt HWND *phwnd){
	(*phwnd) = hWndParent;
	return S_OK;
}
  
HRESULT STDMETHODCALLTYPE Browser::ContextSensitiveHelp(BOOL fEnterMode){
	return E_NOTIMPL;
}
  
// ---------- IOleInPlaceSite ----------
  
HRESULT STDMETHODCALLTYPE Browser::CanInPlaceActivate(void){
	return S_OK;
}  
HRESULT STDMETHODCALLTYPE Browser::OnInPlaceActivate(void){
	OleLockRunning(oleObject, TRUE, FALSE);
	oleObject->QueryInterface(&oleInPlaceObject);
	oleInPlaceObject->SetObjectRects(&rObject, &rObject);  
	return S_OK;  
}  
HRESULT STDMETHODCALLTYPE Browser::OnUIActivate(void){
	return S_OK;
}  
HRESULT STDMETHODCALLTYPE Browser::GetWindowContext(IOleInPlaceFrame **ppFrame,IOleInPlaceUIWindow **ppDoc,LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo){
	HWND hwnd = hWndParent;
  
	(*ppFrame) = NULL;
	(*ppDoc) = NULL;
	(*lprcPosRect).left = rObject.left;
	(*lprcPosRect).top = rObject.top;
	(*lprcPosRect).right = rObject.right;
	(*lprcPosRect).bottom = rObject.bottom;
	*lprcClipRect = *lprcPosRect;
  
	lpFrameInfo->fMDIApp = false;
	lpFrameInfo->hwndFrame = hwnd;
	lpFrameInfo->haccel = NULL;
	lpFrameInfo->cAccelEntries = 0;
  
	return S_OK;
}
  
HRESULT STDMETHODCALLTYPE Browser::Scroll(SIZE scrollExtant){
	return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE Browser::OnUIDeactivate(BOOL fUndoable){
	return S_OK;
}
  
HWND Browser::GetControlWindow(){
	if(hWndControl != 0)
		return hWndControl;
  
	if(oleInPlaceObject == 0)
		return 0;
  
	oleInPlaceObject->GetWindow(&hWndControl);
	return hWndControl;
}
  
HRESULT STDMETHODCALLTYPE Browser::OnInPlaceDeactivate(void){
	hWndControl = 0;
	oleInPlaceObject = 0;
  
	return S_OK;
}
  
HRESULT STDMETHODCALLTYPE Browser::DiscardUndoState(void){
	return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE Browser::DeactivateAndUndo(void){
	return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE Browser::OnPosRectChange(LPCRECT lprcPosRect){
	return E_NOTIMPL;
}
  
// ---------- IOleClientSite ----------
  
HRESULT STDMETHODCALLTYPE Browser::SaveObject(void){
return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE Browser::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk){
	if(	(dwAssign == OLEGETMONIKER_ONLYIFTHERE) &&
		(dwWhichMoniker == OLEWHICHMK_CONTAINER))
		return E_FAIL;  
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::GetContainer(IOleContainer **ppContainer){
	return E_NOINTERFACE;
}  
HRESULT STDMETHODCALLTYPE Browser::ShowObject(void){
	return S_OK;
}  
HRESULT STDMETHODCALLTYPE Browser::OnShowWindow(BOOL fShow){
return S_OK;
}  
HRESULT STDMETHODCALLTYPE Browser::RequestNewObjectLayout(void){
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE Browser::CreateStream(const OLECHAR *pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,IStream **ppstm){
return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::OpenStream(const OLECHAR *pwcsName,void *reserved1,DWORD grfMode,DWORD reserved2,IStream **ppstm){
return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::CreateStorage(const OLECHAR *pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,IStorage **ppstg){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::OpenStorage(const OLECHAR *pwcsName,IStorage *pstgPriority,DWORD grfMode,SNB snbExclude,DWORD reserved,IStorage **ppstg){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::CopyTo(DWORD ciidExclude,const IID *rgiidExclude,SNB snbExclude,IStorage *pstgDest){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::MoveElementTo(const OLECHAR *pwcsName,IStorage *pstgDest,const OLECHAR *pwcsNewName,DWORD grfFlags){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::Commit(DWORD grfCommitFlags){
	return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE Browser::Revert(void){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::EnumElements(DWORD reserved1,void *reserved2,DWORD reserved3,IEnumSTATSTG **ppenum){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::DestroyElement(const OLECHAR *pwcsName){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::RenameElement(const OLECHAR *pwcsOldName,const OLECHAR *pwcsNewName){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::SetElementTimes(const OLECHAR *pwcsName,const FILETIME *pctime,const FILETIME *patime,const FILETIME *pmtime){
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE Browser::SetClass(REFCLSID clsid){
	return S_OK;
}  
HRESULT STDMETHODCALLTYPE Browser::SetStateBits(DWORD grfStateBits,DWORD grfMask){
	return E_NOTIMPL;
}  
HRESULT STDMETHODCALLTYPE Browser::Stat(STATSTG *pstatstg,DWORD grfStatFlag){
	return E_NOTIMPL;
}

#endif