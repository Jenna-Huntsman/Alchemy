/**
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "llstatusbar.h"

// viewer includes
#include "alpanelaopulldown.h"
#include "alpanelquicksettingspulldown.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llfloaterbuycurrency.h"
#include "llbuycurrencyhtml.h"
#include "llpanelnearbymedia.h"
#include "llpanelpresetscamerapulldown.h"
#include "llpanelpresetspulldown.h"
#include "llpanelvolumepulldown.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhints.h"
#include "llhudicon.h"
#include "llnavigationbar.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llrootview.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewermedia.h"
#include "llviewermenu.h"   // for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "lltrans.h"

// library includes
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include "llsearchableui.h"
#include "llsearcheditor.h"

// system includes
#include <iomanip>

//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const S32 SIM_STAT_WIDTH = 8;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY     = 3.f; // How long the balance and health icons should flash after a change.

LLStatusBar::LLStatusBar(const LLRect& rect)
:   LLPanel(),
    mTextTime(NULL),
    mTextFPS(nullptr),
    mPanelPopupHolder(nullptr),
    mBtnQuickSettings(nullptr),
    mBtnAO(nullptr),
    mBtnVolume(NULL),
    mBoxBalance(NULL),
    mBalance(0),
    mHealth(100),
    mSquareMetersCredit(0),
    mSquareMetersCommitted(0),
    mFilterEdit(NULL),          // Edit for filtering
    mSearchPanel(NULL)          // Panel for filtering
{
    setRect(rect);

    // status bar can possible overlay menus?
    setMouseOpaque(FALSE);

    mBalanceTimer = new LLFrameTimer();
    mHealthTimer = new LLFrameTimer();
    mFPSUpdateTimer = new LLFrameTimer();

    buildFromFile("panel_status_bar.xml");
}

LLStatusBar::~LLStatusBar()
{
    delete mBalanceTimer;
    mBalanceTimer = NULL;

    delete mHealthTimer;
    mHealthTimer = NULL;

    // LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
    refresh();
    LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    show_navbar_context_menu(this,x,y);
    return TRUE;
}

BOOL LLStatusBar::postBuild()
{
    gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

    mPanelPopupHolder = gViewerWindow->getRootView()->getChildView("popup_holder");

    mTextTime = getChild<LLTextBox>("TimeText");
    mTextTime->setVisible(gSavedSettings.getBool("ShowStatusBarTime"));

    mBtnBuyL = getChild<LLButton>("buyL");
    mBtnBuyL->setCommitCallback(boost::bind(&LLStatusBar::onClickBuyCurrency, this));

    mBoxBalance = getChild<LLTextBox>("balance");
    mBoxBalance->setClickedCallback( &LLStatusBar::onClickBalance, this );

    mIconPresetsCamera = getChild<LLButton>( "presets_icon_camera" );
    mIconPresetsCamera->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterPresetsCamera, this));

    mIconPresetsGraphic = getChild<LLButton>( "presets_icon_graphic" );
    mIconPresetsGraphic->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterPresets, this));

    mBtnQuickSettings = getChild<LLButton>("quick_settings_btn");
    mBtnQuickSettings->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterQuickSettings, this));

    mBtnAO = getChild<LLButton>("ao_btn");
    mBtnAO->setClickedCallback(&LLStatusBar::onClickAOBtn, this);
    mBtnAO->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterAO, this));
    mBtnAO->setToggleState(gSavedPerAccountSettings.getBOOL("AlchemyAOEnable")); // shunt it into correct state - ALCH-368

    mBtnVolume = getChild<LLButton>( "volume_btn" );
    mBtnVolume->setClickedCallback(&LLStatusBar::onClickVolume, this );
    mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));

    mMediaToggle = getChild<LLButton>("media_toggle_btn");
    mMediaToggle->setClickedCallback( &LLStatusBar::onClickMediaToggle, this );
    mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));

    mBalanceBG = getChild<LLView>("balance_bg");
    LLHints::getInstance()->registerHintTarget("linden_balance", mBalanceBG->getHandle());
    mBoxBalance->setVisible(gSavedSettings.getBool("ShowStatusBarBalance"));

    gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));
// [ALCHEMY]
//    gSavedSettings.getControl("EnableVoiceChat")->getSignal()->connect(boost::bind(&LLStatusBar::onVoiceChanged, this, _2));
//
//    if (!gSavedSettings.getBOOL("EnableVoiceChat") && LLAppViewer::instance()->isSecondInstance())
//    {
//        // Indicate that second instance started without sound
//        mBtnVolume->setImageUnselected(LLUI::getUIImage("VoiceMute_Off"));
//    }
    gSavedPerAccountSettings.getControl("AlchemyAOEnable")->getCommitSignal()->connect(boost::bind(&LLStatusBar::onAOStateChanged, this));

    mTextFPS = getChild<LLTextBox>("FPSText");
    mTextFPS->setClickedCallback([](void*) { LLFloaterReg::showInstance("stats"); });

    mTextFPS->setVisible(gSavedSettings.getBool("ShowStatusBarFPS"));

    mPanelPresetsCameraPulldown = new LLPanelPresetsCameraPulldown();
    addChild(mPanelPresetsCameraPulldown);
    mPanelPresetsCameraPulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelPresetsCameraPulldown->setVisible(FALSE);

    mPanelPresetsPulldown = new LLPanelPresetsPulldown();
    addChild(mPanelPresetsPulldown);
    mPanelPresetsPulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelPresetsPulldown->setVisible(FALSE);

    mPanelVolumePulldown = new LLPanelVolumePulldown();
    addChild(mPanelVolumePulldown);
    mPanelVolumePulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelVolumePulldown->setVisible(FALSE);

    mPanelAOPulldown = new ALPanelAOPulldown();
    addChild(mPanelAOPulldown);
    mPanelAOPulldown->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
    mPanelAOPulldown->setVisible(FALSE);

    mPanelQuickSettingsPulldown = new ALPanelQuickSettingsPulldown();
    addChild(mPanelQuickSettingsPulldown);
    mPanelQuickSettingsPulldown->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
    mPanelQuickSettingsPulldown->setVisible(FALSE);

    mPanelNearByMedia = new LLPanelNearByMedia();
    addChild(mPanelNearByMedia);
    mPanelNearByMedia->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelNearByMedia->setVisible(FALSE);

    updateBalancePanelPosition();

    // Hook up and init for filtering
    mFilterEdit = getChild<LLSearchEditor>( "search_menu_edit" );
    mSearchPanel = getChild<LLPanel>( "menu_search_panel" );

    BOOL search_panel_visible = gSavedSettings.getBOOL("MenuSearch");
    mSearchPanel->setVisible(search_panel_visible);
    mFilterEdit->setKeystrokeCallback(boost::bind(&LLStatusBar::onUpdateFilterTerm, this));
    mFilterEdit->setCommitCallback(boost::bind(&LLStatusBar::onUpdateFilterTerm, this));
    collectSearchableItems();
    gSavedSettings.getControl("MenuSearch")->getCommitSignal()->connect(boost::bind(&LLStatusBar::updateMenuSearchVisibility, this, _2));
    gSavedSettings.getControl("ShowStatusBarTime")->getCommitSignal()->connect(boost::bind(&LLStatusBar::updateClock, this));
    gSavedSettings.getControl("ShowStatusBarSeconds")->getCommitSignal()->connect(boost::bind(&LLStatusBar::updateClock, this));

    if (search_panel_visible)
    {
        updateMenuSearchPosition();
    }

    return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
    static LLCachedControl<bool> show_fps(gSavedSettings, "ShowStatusBarFPS", false);
    static LLCachedControl<bool> show_clock(gSavedSettings, "ShowStatusBarTime", false);
    static LLCachedControl<bool> show_clock_seconds(gSavedSettings, "ShowStatusBarSeconds", false);

    // update clock every 10 seconds
    if(show_clock && (mClockUpdateTimer.getElapsedTimeF32() > 10.f || (show_clock_seconds && mClockUpdateTimer.getElapsedTimeF32() > 1.f)))
    {
        mClockUpdateTimer.reset();

        updateClock();
    }

    LLRect r;
    const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();

    // reshape menu bar to its content's width
    if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
    {
        gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
    }

    // update the master volume button state
    bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
    mBtnVolume->setToggleState(mute_audio);

    LLViewerMedia* media_inst = LLViewerMedia::getInstance();

    // Disable media toggle if there's no media, parcel media, and no parcel audio
    // (or if media is disabled)
    static const LLCachedControl<bool> audio_streaming_enabled(gSavedSettings, "AudioStreamingMusic");
    static const LLCachedControl<bool> media_streaming_enabled(gSavedSettings, "AudioStreamingMedia");
    bool button_enabled = (audio_streaming_enabled || media_streaming_enabled) &&
                          (media_inst->hasInWorldMedia() || media_inst->hasParcelMedia() || media_inst->hasParcelAudio());
    mMediaToggle->setEnabled(button_enabled);
    // Note the "sense" of the toggle is opposite whether media is playing or not
    bool any_media_playing = (media_inst->isAnyMediaPlaying() ||
                              media_inst->isParcelMediaPlaying() ||
                              media_inst->isParcelAudioPlaying());
    mMediaToggle->setValue(!any_media_playing);

    if (show_fps && mFPSUpdateTimer->getElapsedTimeF32() > 0.125f)
    {
        mFPSUpdateTimer->reset();
        F32 fps = (F32)LLTrace::get_frame_recording().getLastRecording().getMean(LLStatViewer::FPS_SAMPLE);
        mTextFPS->setText(fmt::format(FMT_STRING("{:d}"), llfloor(fps)));
    }
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
    static LLCachedControl<bool> show_balance(gSavedSettings, "ShowStatusBarBalance", false);
    static LLCachedControl<bool> show_clock(gSavedSettings, "ShowStatusBarTime", false);
    static LLCachedControl<bool> show_fps(gSavedSettings, "ShowStatusBarFPS", false);
    static LLCachedControl<bool> show_menu_search(gSavedSettings, "MenuSearch", false);
    mTextTime->setVisible(visible && show_clock);
    mBalanceBG->setVisible(visible && show_balance);
    mBtnQuickSettings->setVisible(visible);
    mBtnAO->setVisible(visible);
    mBtnVolume->setVisible(visible);
    mMediaToggle->setVisible(visible);
    mSearchPanel->setVisible(visible && show_menu_search);
    setBackgroundVisible(visible);
    mIconPresetsCamera->setVisible(visible);
    mIconPresetsGraphic->setVisible(visible);
    mTextFPS->setVisible(visible && show_fps);
}

void LLStatusBar::debitBalance(S32 debit)
{
    setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
    setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
    if (balance > getBalance() && getBalance() != 0)
    {
        LLFirstUse::receiveLindens();
    }

    std::string money_str = LLResMgr::getMonetaryString( balance );

    LLStringUtil::format_map_t string_args;
    string_args["[AMT]"] = llformat("%s", money_str.c_str());
    std::string label_str = getString("buycurrencylabel", string_args);
    mBoxBalance->setValue(label_str);

    updateBalancePanelPosition();

    // If the search panel is shown, move this according to the new balance width. Parcel text will reshape itself in setParcelInfoText
    if (mSearchPanel && mSearchPanel->getVisible())
    {
        updateMenuSearchPosition();
    }

    if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
    {
        if (mBalance > balance)
            make_ui_sound("UISndMoneyChangeDown");
        else
            make_ui_sound("UISndMoneyChangeUp");
    }

    if( balance != mBalance )
    {
        mBalanceTimer->reset();
        mBalanceTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
        mBalance = balance;
    }
}


// static
void LLStatusBar::sendMoneyBalanceRequest()
{
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_MoneyData);
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );

    if (gDisconnected)
    {
        LL_DEBUGS() << "Trying to send message when disconnected, skipping balance request!" << LL_ENDL;
        return;
    }
    if (!gAgent.getRegion())
    {
        LL_DEBUGS() << "LLAgent::sendReliableMessage No region for agent yet, skipping balance request!" << LL_ENDL;
        return;
    }
    // Double amount of retries due to this request initially happening during busy stage
    // Ideally this should be turned into a capability
    gMessageSystem->sendReliable(gAgent.getRegionHost(), LL_DEFAULT_RELIABLE_RETRIES * 2, TRUE, LL_PING_BASED_TIMEOUT_DUMMY, NULL, NULL);
}


void LLStatusBar::setHealth(S32 health)
{
    //LL_INFOS() << "Setting health to: " << buffer << LL_ENDL;
    if( mHealth > health )
    {
        if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
        {
            if (isAgentAvatarValid())
            {
                if (gAgentAvatarp->getSex() == SEX_FEMALE)
                {
                    make_ui_sound("UISndHealthReductionF");
                }
                else
                {
                    make_ui_sound("UISndHealthReductionM");
                }
            }
        }

        mHealthTimer->reset();
        mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
    }

    mHealth = health;
}

S32 LLStatusBar::getBalance() const
{
    return mBalance;
}


S32 LLStatusBar::getHealth() const
{
    return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
    mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
    mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
    return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
    return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
    return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
    return mSquareMetersCredit - mSquareMetersCommitted;
}

void LLStatusBar::onClickBuyCurrency()
{
    // open a currency floater - actual one open depends on
    // value specified in settings.xml
    LLBuyCurrencyHTML::openCurrencyFloater();
    LLFirstUse::receiveLindens(false);
}

void LLStatusBar::onMouseEnterPresetsCamera()
{
    LLIconCtrl* icon =  getChild<LLIconCtrl>( "presets_icon_camera" );
    LLRect icon_rect = icon->getRect();
    LLRect pulldown_rect = mPanelPresetsCameraPulldown->getRect();
    pulldown_rect.setLeftTopAndSize(icon_rect.mLeft -
         (pulldown_rect.getWidth() - icon_rect.getWidth()),
                   icon_rect.mBottom,
                   pulldown_rect.getWidth(),
                   pulldown_rect.getHeight());

    pulldown_rect.translate(mPanelPopupHolder->getRect().getWidth() - pulldown_rect.mRight, 0);
    mPanelPresetsCameraPulldown->setShape(pulldown_rect);

    // show the master presets pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelPresetsCameraPulldown);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(FALSE);
    // mPanelAvatarComplexityPulldown->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(FALSE);
    mPanelPresetsCameraPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterPresets()
{
    LLIconCtrl* icon =  getChild<LLIconCtrl>( "presets_icon_graphic" );
    LLRect icon_rect = icon->getRect();
    LLRect pulldown_rect = mPanelPresetsPulldown->getRect();
    pulldown_rect.setLeftTopAndSize(icon_rect.mLeft -
         (pulldown_rect.getWidth() - icon_rect.getWidth()),
                   icon_rect.mBottom,
                   pulldown_rect.getWidth(),
                   pulldown_rect.getHeight());

    pulldown_rect.translate(mPanelPopupHolder->getRect().getWidth() - pulldown_rect.mRight, 0);
    mPanelPresetsPulldown->setShape(pulldown_rect);

    // show the master presets pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelPresetsPulldown);

    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(FALSE);
    // mPanelAvatarComplexityPulldown->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterQuickSettings()
{
    LLRect qs_rect = mPanelQuickSettingsPulldown->getRect();
    LLRect qs_btn_rect = mBtnQuickSettings->getRect();
    qs_rect.setLeftTopAndSize(qs_btn_rect.mLeft -
        (qs_rect.getWidth() - qs_btn_rect.getWidth()) / 2,
        qs_btn_rect.mBottom,
        qs_rect.getWidth(),
        qs_rect.getHeight());
    // force onscreen
    qs_rect.translate(mPanelPopupHolder->getRect().getWidth() - qs_rect.mRight, 0);

    // show the master volume pull-down
    mPanelQuickSettingsPulldown->setShape(qs_rect);
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelQuickSettingsPulldown);

    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(FALSE);
    //mPanelAvatarComplexityPulldown->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterAO()
{
    LLRect qs_rect = mPanelAOPulldown->getRect();
    LLRect qs_btn_rect = mBtnAO->getRect();
    qs_rect.setLeftTopAndSize(qs_btn_rect.mLeft -
                              (qs_rect.getWidth() - qs_btn_rect.getWidth()) / 2,
                              qs_btn_rect.mBottom,
                              qs_rect.getWidth(),
                              qs_rect.getHeight());
    // force onscreen
    qs_rect.translate(mPanelPopupHolder->getRect().getWidth() - qs_rect.mRight, 0);

    mPanelAOPulldown->setShape(qs_rect);
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelAOPulldown);

    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(TRUE);
    //mPanelAvatarComplexityPulldown->setVisible(FALSE);
}

void LLStatusBar::onMouseEnterVolume()
{
    LLButton* volbtn =  getChild<LLButton>( "volume_btn" );
    LLRect vol_btn_rect = volbtn->getRect();
    LLRect volume_pulldown_rect = mPanelVolumePulldown->getRect();
    volume_pulldown_rect.setLeftTopAndSize(vol_btn_rect.mLeft -
         (volume_pulldown_rect.getWidth() - vol_btn_rect.getWidth()),
                   vol_btn_rect.mBottom,
                   volume_pulldown_rect.getWidth(),
                   volume_pulldown_rect.getHeight());

    volume_pulldown_rect.translate(mPanelPopupHolder->getRect().getWidth() - volume_pulldown_rect.mRight, 0);
    mPanelVolumePulldown->setShape(volume_pulldown_rect);


    // show the master volume pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelVolumePulldown);
    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterNearbyMedia()
{
    LLRect nearby_media_rect = mPanelNearByMedia->getRect();
    LLButton* nearby_media_btn =  getChild<LLButton>( "media_toggle_btn" );
    LLRect nearby_media_btn_rect = nearby_media_btn->getRect();
    nearby_media_rect.setLeftTopAndSize(nearby_media_btn_rect.mLeft -
                                        (nearby_media_rect.getWidth() - nearby_media_btn_rect.getWidth())/2,
                                        nearby_media_btn_rect.mBottom,
                                        nearby_media_rect.getWidth(),
                                        nearby_media_rect.getHeight());
    // force onscreen
    nearby_media_rect.translate(mPanelPopupHolder->getRect().getWidth() - nearby_media_rect.mRight, 0);

    // show the master volume pull-down
    mPanelNearByMedia->setShape(nearby_media_rect);
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelNearByMedia);

    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelQuickSettingsPulldown->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelAOPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(TRUE);
}


// static
void LLStatusBar::onClickAOBtn(void* data)
{
    gSavedPerAccountSettings.set("AlchemyAOEnable", !gSavedPerAccountSettings.getBOOL("AlchemyAOEnable"));
}

// static
void LLStatusBar::onClickVolume(void* data)
{
    // toggle the master mute setting
    bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
    LLAppViewer::instance()->setMasterSystemAudioMute(!mute_audio);
}

//static
void LLStatusBar::onClickBalance(void* )
{
    // Force a balance request message:
    LLStatusBar::sendMoneyBalanceRequest();
    // The refresh of the display (call to setBalance()) will be done by process_money_balance_reply()
}

//static
void LLStatusBar::onClickMediaToggle(void* data)
{
    LLStatusBar *status_bar = (LLStatusBar*)data;
    // "Selected" means it was showing the "play" icon (so media was playing), and now it shows "pause", so turn off media
    bool pause = status_bar->mMediaToggle->getValue();
    LLViewerMedia::getInstance()->setAllMediaPaused(pause);
}

void LLStatusBar::onAOStateChanged()
{
    mBtnAO->setToggleState(gSavedPerAccountSettings.getBOOL("AlchemyAOEnable"));
}

BOOL can_afford_transaction(S32 cost)
{
    return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
    refresh();
}

// [ALCHEMY]
//void LLStatusBar::onVoiceChanged(const LLSD& newvalue)
//{
//    if (newvalue.asBoolean())
//    {
//        // Second instance starts with "VoiceMute_Off" icon, fix it
//        mBtnVolume->setImageUnselected(LLUI::getUIImage("Audio_Off"));
//    }
//    refresh();
//}

void LLStatusBar::onUpdateFilterTerm()
{
    LLWString searchValue = utf8str_to_wstring( mFilterEdit->getValue().asString() );
    LLWStringUtil::toLower( searchValue );

    if( !mSearchData || mSearchData->mLastFilter == searchValue )
        return;

    mSearchData->mLastFilter = searchValue;

    mSearchData->mRootMenu->hightlightAndHide( searchValue );
    gMenuBarView->needsArrange();
}

void collectChildren( LLMenuGL *aMenu, ll::statusbar::SearchableItemPtr aParentMenu )
{
    for( U32 i = 0; i < aMenu->getItemCount(); ++i )
    {
        LLMenuItemGL *pMenu = aMenu->getItem( i );

        ll::statusbar::SearchableItemPtr pItem( new ll::statusbar::SearchableItem );
        pItem->mCtrl = pMenu;
        pItem->mMenu = pMenu;
        pItem->mLabel = utf8str_to_wstring( pMenu->ll::ui::SearchableControl::getSearchText() );
        LLWStringUtil::toLower( pItem->mLabel );
        aParentMenu->mChildren.push_back( pItem );

        LLMenuItemBranchGL *pBranch = dynamic_cast< LLMenuItemBranchGL* >( pMenu );
        if( pBranch )
            collectChildren( pBranch->getBranch(), pItem );
    }

}

void LLStatusBar::collectSearchableItems()
{
    mSearchData.reset( new ll::statusbar::SearchData );
    ll::statusbar::SearchableItemPtr pItem( new ll::statusbar::SearchableItem );
    mSearchData->mRootMenu = pItem;
    collectChildren( gMenuBarView, pItem );
}

void LLStatusBar::updateMenuSearchVisibility(const LLSD& data)
{
    bool visible = data.asBoolean();
    mSearchPanel->setVisible(visible);
    if (!visible)
    {
        mFilterEdit->setText(LLStringUtil::null);
        onUpdateFilterTerm();
    }
    else
    {
        updateMenuSearchPosition();
    }
}

void LLStatusBar::updateMenuSearchPosition()
{
    const S32 HPAD = 12;
    LLRect balanceRect = mBalanceBG->getRect();
    LLRect searchRect = mSearchPanel->getRect();
    S32 w = searchRect.getWidth();
    searchRect.mLeft = balanceRect.mLeft - w - HPAD;
    searchRect.mRight = searchRect.mLeft + w;
    mSearchPanel->setShape( searchRect );
}

void LLStatusBar::updateBalancePanelPosition()
{
    // Resize the L$ balance background to be wide enough for your balance plus the buy button
    const S32 HPAD = 24;
    LLRect balance_rect = mBoxBalance->getTextBoundingRect();
    LLRect buy_rect = mBtnBuyL->getRect();
    LLRect balance_bg_rect = mBalanceBG->getRect();
    balance_bg_rect.mLeft = balance_bg_rect.mRight - (buy_rect.getWidth() + balance_rect.getWidth() + HPAD);
    mBalanceBG->setShape(balance_bg_rect);
}

void LLStatusBar::updateClock()
{
    static LLCachedControl<bool> show_clock_seconds(gSavedSettings, "ShowStatusBarSeconds", false);

    // Get current UTC time, adjusted for the user's clock
    // being off.
    time_t utc_time;
    utc_time = time_corrected();

    static const std::string timeStrTemplate = getString("time");
    static const std::string timeStrSecondsTemplate = getString("timeSeconds");
    std::string timeStr = show_clock_seconds ? timeStrSecondsTemplate : timeStrTemplate;
    LLSD substitution;
    substitution["datetime"] = (S32)utc_time;
    LLStringUtil::format(timeStr, substitution);
    mTextTime->setText(timeStr);

    // set the tooltip to have the date
    static const std::string dtStrTemplate = getString("timeTooltip");
    std::string dtStr = dtStrTemplate;
    LLStringUtil::format(dtStr, substitution);
    mTextTime->setToolTip(dtStr);
}

// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
    // Requires "trusted" browser/URL source
    LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
    bool handle(const LLSD& tokens, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web)
    {
        if (tokens.size() == 1
            && tokens[0].asString() == "request")
        {
            LLStatusBar::sendMoneyBalanceRequest();
            return true;
        }
        return false;
    }
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
