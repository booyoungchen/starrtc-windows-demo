// CVoipDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CVoipDlg.h"
#include "afxdialogex.h"
#include "CropType.h"
#include "CNewDlg.h"
#include "CHistoryBean.h"
enum VOIP_LIST_REPORT_NAME
{
	VOIP_VIDEO_NAME = 0,
	VOIP_VIDEO_STATUS,
	VOIP_VIDEO_ID,
	VOIP_VIDEO_CREATER
};
// CVoipDlg 对话框

IMPLEMENT_DYNAMIC(CVoipDlg, CDialogEx)

CVoipDlg::CVoipDlg(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_VOIP_DLG, pParent)
{
	m_pUserManager = pUserManager;
	m_pVoipManager = new XHVoipManager(this);
	m_ShowLiveDlg.addPictureControlListener(this);
	m_ShowLiveDlg.addShowLiveListener(this);
	m_pSqliteDB = new CSqliteDB();
	m_pSqliteDB->openDB("chatDB.db");
	m_strTargetId = "";
	m_pSoundManager = new CSoundManager(this);
	//m_ShowLiveDlg.m_pShowLiveControl->m_showType = LIVE_SHOW_TYPE_VOIP;
}

CVoipDlg::~CVoipDlg()
{
	if (m_pVoipManager != NULL)
	{
		delete m_pVoipManager;
		m_pVoipManager = NULL;
	}
	stopGetData();
	clearHistoryList();
	if (m_pSoundManager != NULL)
	{
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}
	/*if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}*/

}

void CVoipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VOIP_SHOW_AREA, m_ShowArea);
	DDX_Control(pDX, IDC_LIST_VOIP, m_VoipList);
	DDX_Control(pDX, IDC_STATIC_VOIP_USER_ID, m_UserId);
	DDX_Control(pDX, IDC_EDIT_VOIP_SEND_MSG, m_editSendMsg);
	DDX_Control(pDX, IDC_LIST_VOIP_HISTORY_MSG, m_HistoryMsg);
}


BEGIN_MESSAGE_MAP(CVoipDlg, CDialogEx)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON_CREATE_VOIP, &CVoipDlg::OnBnClickedButtonCreateVoip)
	ON_NOTIFY(NM_CLICK, IDC_LIST_VOIP, &CVoipDlg::OnNMClickListVoip)
	ON_BN_CLICKED(IDC_BUTTON_VOIP_SEND_MSG, &CVoipDlg::OnBnClickedButtonVoipSendMsg)
	ON_BN_CLICKED(IDC_BUTTON_VOIP_START_LIVE, &CVoipDlg::OnBnClickedButtonVoipStartLive)
	ON_BN_CLICKED(IDC_BUTTON_VOIP_START_AUDIO_LIVE, &CVoipDlg::OnBnClickedButtonVoipStartAudioLive)
END_MESSAGE_MAP()


// CVoipDlg 消息处理程序


BOOL CVoipDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	LONG lStyle;
	lStyle = GetWindowLong(m_VoipList.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_VoipList.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyleLiveList = m_VoipList.GetExtendedStyle();
	dwStyleLiveList |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyleLiveList |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_VoipList.SetExtendedStyle(dwStyleLiveList);

	m_VoipList.InsertColumn(VOIP_VIDEO_NAME, _T("ID"), LVCFMT_LEFT, 110);
	//m_liveListListControl.InsertColumn(MEETING_NAME, _T("Name"), LVCFMT_LEFT, 120);
	//m_liveListListControl.InsertColumn(MEETING_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	m_VoipList.InsertColumn(VOIP_VIDEO_STATUS, _T("liveState"), LVCFMT_LEFT, 100);
	
	m_ShowLiveDlg.Create(IDD_DIALOG_SHOW_LIVE, this);

	getHistoryList();
	resetHistoryList();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
void CVoipDlg::clearHistoryList()
{
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		if (*it != NULL)
		{
			delete *it;
			*it = NULL;
		}
	}
	mHistoryDatas.clear();
}

void CVoipDlg::getHistoryList()
{
	clearHistoryList();
	if (m_pSqliteDB != NULL)
	{
		list<CHistoryBean*> historyList = m_pSqliteDB->getHistory(HISTORY_TYPE_VOIP);
		if (historyList.size() > 0)
		{
			mHistoryDatas.splice(mHistoryDatas.begin(), historyList);
		}
	}
}

void CVoipDlg::resetHistoryList()
{
	m_VoipList.DeleteAllItems();

	int nRow = 0;
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		m_VoipList.InsertItem(0, (*it)->getConversationId().c_str());
	}
}

void CVoipDlg::addHistoryList(CHistoryBean* pHistoryBean)
{
	CHistoryBean* pFindHistory = NULL;
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		if ((*it)->getConversationId() == pHistoryBean->getConversationId())
		{
			pFindHistory = *it;
			break;
		}
	}

	if (pFindHistory != NULL)
	{
		pFindHistory->setLastMsg(pHistoryBean->getLastMsg());
		pFindHistory->setLastTime(pHistoryBean->getLastTime());
		delete pHistoryBean;
		pHistoryBean = NULL;
	}
	else
	{
		if (m_pSqliteDB != NULL)
		{
			m_pSqliteDB->setHistory(pHistoryBean, true);
		}
		mHistoryDatas.push_front(pHistoryBean);
	}
}

void CVoipDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()
/*	for (int i = 0; i < 2; i++)
	{
		if (m_pDataShowView != NULL)
		{
			CDC* pDC = m_pDataShowView->m_pPictureControlArr[0]->GetWindowDC();    //获得显示控件的DC
			pDC->SetStretchBltMode(COLORONCOLOR);
			FillRect(pDC->m_hDC, &m_pDataShowView->m_DrawRect, CBrush(RGB(0, 0, 0)));
			m_pDataShowView->m_pPictureControlArr[0]->ReleaseDC(pDC);
		}
	}*/
}

// voip callback
/**
 * 被叫方响应
 * 被叫方收到主叫方的呼叫
 * @param fromID
 */
void CVoipDlg::onCalling(string fromID)
{
	CString str;
	str.Format("是否同意用户:%s请求视频通话", fromID.c_str());
	if (IDYES == AfxMessageBox(str, MB_YESNO))
	{
		m_bAudio = false;
		m_strTargetId = fromID;
		CRect rect;
		::GetWindowRect(this->m_hWnd, rect);
		m_ShowLiveDlg.MoveWindow(CRect(rect.right, rect.top, rect.right + rect.Width() / 2, rect.bottom), true);
		m_ShowLiveDlg.ShowWindow(SW_SHOW);
		m_pVoipManager->accept(fromID);
		m_bConnect = true;

		if (m_ShowLiveDlg.m_pDataShowView != NULL)
		{
			m_ShowLiveDlg.m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId,false);
			m_ShowLiveDlg.m_pDataShowView->addUser(fromID, true);
			m_ShowLiveDlg.m_pDataShowView->setShowPictures();
		}
		CHistoryBean* pHistoryBean = new CHistoryBean();
		pHistoryBean->setType(HISTORY_TYPE_VOIP);
		pHistoryBean->setLastTime(CUtil::getTime());
		pHistoryBean->setConversationId(fromID);
		pHistoryBean->setNewMsgCount(1);
		addHistoryList(pHistoryBean);
		startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->startSoundData(true);
		}			
	}
	else
	{
		m_pVoipManager->refuse();
	}
}

/**
* 被叫方响应语音呼叫
* 被叫方收到主叫方的呼叫
* @param fromID
*/
void CVoipDlg::onAudioCalling(string fromID)
{
	CString str;
	str.Format("是否同意用户:%s请求语音通话", fromID.c_str());
	if (IDYES == AfxMessageBox(str, MB_YESNO))
	{
		m_bAudio = true;
		m_strTargetId = fromID;
		CRect rect;
		::GetWindowRect(this->m_hWnd, rect);
		m_ShowLiveDlg.m_bAudio = true;
		m_ShowLiveDlg.m_strAimID = fromID;
		m_ShowLiveDlg.MoveWindow(CRect(rect.right, rect.top, rect.right + rect.Width() / 2, rect.bottom), true);
		m_ShowLiveDlg.ShowWindow(SW_SHOW);
		m_pVoipManager->accept(fromID);
		m_bConnect = true;

		if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
		{
			m_ShowLiveDlg.m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId, false);
			m_ShowLiveDlg.m_pDataShowView->addUser(fromID, true);
			m_ShowLiveDlg.m_pDataShowView->setShowPictures();
		}
		CHistoryBean* pHistoryBean = new CHistoryBean();
		pHistoryBean->setType(HISTORY_TYPE_VOIP);
		pHistoryBean->setLastTime(CUtil::getTime());
		pHistoryBean->setConversationId(fromID);
		pHistoryBean->setNewMsgCount(1);
		addHistoryList(pHistoryBean);
		//startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->startSoundData(true);
		}
	}
	else
	{
		m_pVoipManager->refuse();
	}
}

/**
 * 被叫方响应
 * 主叫方在被叫方接听之前挂断（通话被取消）
 * @param fromID
 */
void CVoipDlg::onCancled(string fromID)
{
	stopGetData();
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	m_strTargetId = "";
}

/**
 * 主叫方响应
 * 被叫方拒绝接通（通话被拒绝）
 * @param fromID
 */
void CVoipDlg::onRefused(string fromID)
{
	stopGetData();
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	m_strTargetId = "";
	AfxMessageBox("对方拒绝接通");
}

/**
 * 主叫方响应
 * 被叫方线路忙（对方通话中）
 * @param fromID
 */
void CVoipDlg::onBusy(string fromID)
{
	//关闭
	stopGetData();
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	m_strTargetId = "";
	AfxMessageBox("对方线路忙");
}

/**
 * 通话中错过的来电
 */
void CVoipDlg::onMiss(CIMMessage* pMsg)
{
}

/**
 * 主叫方响应
 * 被叫方接通（通话开始）
 * @param fromID
 */
void CVoipDlg::onConnected(string fromID)
{
	//添加显示对方界面
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_strTargetId = fromID;
		m_ShowLiveDlg.m_pDataShowView->addUser(fromID, true);
		m_ShowLiveDlg.m_pDataShowView->changeShowStyle(m_pUserManager->m_ServiceParam.m_strUserId, false);
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	//设置插入数据
	m_bInsertData = true;
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->startSoundData(true);
	}
	m_bConnect = true;
}

/**
 * 对方已经挂断
 * @param fromID
 */
void CVoipDlg::onHangup(string fromID)
{
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}

	if (m_pVoipManager != NULL)
	{
		if (m_bConnect)
		{
			m_pVoipManager->hangup(0);
		}
		else
		{
			m_pVoipManager->cancel();
		}
		m_bConnect = false;
	}
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	m_strTargetId = "";
}

/**
 * voip报错
 * @param errorCode
 */
void CVoipDlg::onError(string errorCode)
{
	//开启自己
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pVoipManager != NULL)
	{
		m_pVoipManager->hangup(0);
	}
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	//断开连接
	CString strErr;
	strErr.Format("err:%s", errorCode.c_str());
	AfxMessageBox(strErr);
	m_strTargetId = "";
}

/**
 * 收到实时数据
 * @param data
 */
void CVoipDlg::onReceiveRealtimeData(uint8_t* data, int length)
{
}

int CVoipDlg::getVideoRaw(string strUserId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->drawPic(FMT_YUV420P, m_strTargetId, w, h, videoData, videoDataLen);
	}
	return 0;
}

/**
 * 数据传输模式切换
 * @param state
 */
void CVoipDlg::onTransStateChanged(int state)
{
}

void CVoipDlg::stopLive()
{
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}

	if (m_pVoipManager != NULL)
	{
		if (m_bConnect)
		{
			m_pVoipManager->hangup(1);
		}
		else
		{
			m_pVoipManager->cancel();
		}
		m_bConnect = false;
	}
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->removeAllUser();
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
	m_ShowLiveDlg.ShowWindow(SW_HIDE);
	m_strTargetId = "";
}

void CVoipDlg::addUpId()
{
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId, false);
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
}

void CVoipDlg::insertVideoRaw(uint8_t* videoData, int dataLen, int isBig)
{
	if (m_pVoipManager != NULL && m_bAudio == false)
	{
		m_pVoipManager->insertVideoRaw(videoData, dataLen, isBig);
	}
}

int CVoipDlg::cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall)
{
	int ret = 0;
	if (m_pVoipManager != NULL)
	{
		ret = m_pVoipManager->cropVideoRawNV12(w, h, videoData, dataLen, (int)CROP_TYPE::VIDEO_CROP_CONFIG_480BW_480BH_SMALL_NONE, 0, 0, outVideoDataBig, outVideoDataSmall);
	}
	return ret;
}
void CVoipDlg::drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_ShowLiveDlg.m_pDataShowView != NULL && m_bAudio == false)
	{
		m_ShowLiveDlg.m_pDataShowView->drawPic(type, m_pUserManager->m_ServiceParam.m_strUserId, w, h, videoData, videoDataLen);
	}
}

void CVoipDlg::onNewMessage(CIMMessage* var1)
{
	CString strMsg = "";
	strMsg.Format("%s:%s", var1->m_strFromId.c_str(), var1->m_strContentData.c_str());
	m_HistoryMsg.InsertString(0, strMsg);
}

void CVoipDlg::changeShowStyle(string strUserId)
{
	if (m_ShowLiveDlg.m_pDataShowView != NULL)
	{
		string strId = m_ShowLiveDlg.m_pDataShowView->changeShowStyle(strUserId, true);
		m_ShowLiveDlg.m_pDataShowView->changeShowStyle(strId, false);
		m_ShowLiveDlg.m_pDataShowView->setShowPictures();
	}
}
void CVoipDlg::closeCurrentLive()
{
}
void CVoipDlg::getLocalSoundData(char* pData, int nLength)
{
	if (m_pVoipManager != NULL)
	{
		m_pVoipManager->insertAudioRaw((uint8_t*)pData, nLength);
	}
}

void CVoipDlg::querySoundData(char** pData, int* nLength)
{
	if (m_pVoipManager != NULL)
	{
		m_pVoipManager->querySoundData((uint8_t**)pData, nLength);
	}
}

void CVoipDlg::OnBnClickedButtonCreateVoip()
{
	CNewDlg dlg;
	dlg.m_strText = "请输入用户Id";
	dlg.m_strSureButton = "确定";
	dlg.m_strTitle = "创建一对一直播";
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_strContent;
		m_UserId.SetWindowText(dlg.m_strContent);
		m_VoipList.InsertItem(0, dlg.m_strContent);
	}
}


void CVoipDlg::OnNMClickListVoip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString str = m_VoipList.GetItemText(nItem, VOIP_VIDEO_NAME);
			m_UserId.SetWindowText(str);
		}
	}
}


void CVoipDlg::OnBnClickedButtonVoipSendMsg()
{
/*	CString str = "";
	CString strMsg = "";
	m_UserId.GetWindowText(str);
	m_editSendMsg.GetWindowText(strMsg);
	if ((!str.IsEmpty()) && (!strMsg.IsEmpty()))
	{
		if (m_pVoipManager != NULL)
		{
			m_pVoipManager->sendMsg(str.GetBuffer(0), strMsg.GetBuffer(0));
		}
		CString msgInfo;
		msgInfo.Format("%s:%s", m_pUserManager->m_ServiceParam.m_strUserId.c_str(), strMsg);
		m_HistoryMsg.InsertString(0, msgInfo);
	}
	m_editSendMsg.SetSel(0, -1); // 选中所有字符
	m_editSendMsg.ReplaceSel(_T(""));*/
}


void CVoipDlg::OnBnClickedButtonVoipStartLive()
{
	CString strUserId;
	m_UserId.GetWindowText(strUserId);
	if (!strUserId.IsEmpty())
	{
		//呼叫对方
		if (m_pVoipManager != NULL)
		{
			m_pVoipManager->call(strUserId.GetBuffer(0));
		}

		CHistoryBean* pHistoryBean = new CHistoryBean();
		pHistoryBean->setType(HISTORY_TYPE_VOIP);
		pHistoryBean->setLastTime(CUtil::getTime());
		pHistoryBean->setConversationId(strUserId.GetBuffer(0));
		pHistoryBean->setNewMsgCount(1);
		addHistoryList(pHistoryBean);

		CRect rect;
		::GetWindowRect(this->m_hWnd, rect);
		m_ShowLiveDlg.MoveWindow(CRect(rect.right, rect.top, rect.right + rect.Width() / 2, rect.bottom), true);
		m_ShowLiveDlg.ShowWindow(SW_SHOW);

		if (m_ShowLiveDlg.m_pDataShowView != NULL)
		{
			m_ShowLiveDlg.m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId, true);
			m_ShowLiveDlg.m_pDataShowView->setShowPictures();
		}

		startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, false);
	}
}


void CVoipDlg::OnBnClickedButtonVoipStartAudioLive()
{
	CString strUserId;
	m_UserId.GetWindowText(strUserId);
	if (!strUserId.IsEmpty())
	{
		m_bAudio = true;
		//呼叫对方
		if (m_pVoipManager != NULL)
		{
			m_pVoipManager->audioCall(strUserId.GetBuffer(0));
		}

		CHistoryBean* pHistoryBean = new CHistoryBean();
		pHistoryBean->setType(HISTORY_TYPE_VOIP);
		pHistoryBean->setLastTime(CUtil::getTime());
		pHistoryBean->setConversationId(strUserId.GetBuffer(0));
		pHistoryBean->setNewMsgCount(1);
		addHistoryList(pHistoryBean);

		CRect rect;
		::GetWindowRect(this->m_hWnd, rect);
		m_ShowLiveDlg.m_bAudio = true;
		m_ShowLiveDlg.m_strAimID = strUserId.GetBuffer(0);
		m_ShowLiveDlg.MoveWindow(CRect(rect.right, rect.top, rect.right + rect.Width() / 2, rect.bottom), true);
		m_ShowLiveDlg.ShowWindow(SW_SHOW);

		/*if (m_ShowLiveDlg.m_pDataShowView != NULL)
		{
			m_ShowLiveDlg.m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId, true);
			m_ShowLiveDlg.m_pDataShowView->setShowPictures();
		}*/

		//startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, false);
	}
}
