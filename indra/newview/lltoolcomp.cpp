/**
 * @file lltoolcomp.cpp
 * @brief Composite tools
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lltoolcomp.h"

#include "llfloaterreg.h"
#include "llgl.h"
#include "indra_constants.h"

#include "llmanip.h"
#include "llmaniprotate.h"
#include "llmanipscale.h"
#include "llmaniptranslate.h"
#include "llmenugl.h"           // for right-click menu hack
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolgun.h"
#include "lltoolmgr.h"
#include "lltoolselectrect.h"
#include "lltoolplacer.h"
#include "llviewerinput.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfloatertools.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "lleventtimer.h"

// we use this in various places instead of NULL
static LLPointer<LLTool> sNullTool(new LLTool(std::string("null"), NULL));

//-----------------------------------------------------------------------
// LLToolComposite

//static
void LLToolComposite::setCurrentTool( LLTool* new_tool )
{
    if( mCur != new_tool )
    {
        if( mSelected )
        {
            mCur->handleDeselect();
            mCur = new_tool;
            mCur->handleSelect();
        }
        else
        {
            mCur = new_tool;
        }
    }
}

LLToolComposite::LLToolComposite(const std::string& name)
    : LLTool(name),
      mCur(sNullTool),
      mDefault(sNullTool),
      mSelected(FALSE),
      mMouseDown(FALSE), mManip(NULL), mSelectRect(NULL)
{
}

// Returns to the default tool
BOOL LLToolComposite::handleMouseUp(S32 x, S32 y, MASK mask)
{
    BOOL handled = mCur->handleMouseUp( x, y, mask );
    if( handled )
    {
        setCurrentTool( mDefault );
    }
 return handled;
}

void LLToolComposite::onMouseCaptureLost()
{
    mCur->onMouseCaptureLost();
    setCurrentTool( mDefault );
}

BOOL LLToolComposite::isSelecting()
{
    return mCur == mSelectRect;
}

void LLToolComposite::handleSelect()
{
    if (!ALControlCache::EditLinkedParts)
    {
        LLSelectMgr::getInstance()->promoteSelectionToRoot();
    }
    mCur = mDefault;
    mCur->handleSelect();
    mSelected = TRUE;
}

void LLToolComposite::handleDeselect()
{
    mCur->handleDeselect();
    mCur = mDefault;
    mSelected = FALSE;
}

//----------------------------------------------------------------------------
// LLToolCompInspect
//----------------------------------------------------------------------------

LLToolCompInspect::LLToolCompInspect()
: LLToolComposite(std::string("Inspect")),
  mIsToolCameraActive(FALSE)
{
    mSelectRect     = new LLToolSelectRect(this);
    mDefault = mSelectRect;
}


LLToolCompInspect::~LLToolCompInspect()
{
    delete mSelectRect;
    mSelectRect = NULL;
}

BOOL LLToolCompInspect::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL handled = FALSE;

    if (mCur == LLToolCamera::getInstance())
    {
        handled = mCur->handleMouseDown(x, y, mask);
    }
    else
    {
        mMouseDown = TRUE;
        gViewerWindow->pickAsync(x, y, mask, pickCallback);
        handled = TRUE;
    }

    return handled;
}

BOOL LLToolCompInspect::handleMouseUp(S32 x, S32 y, MASK mask)
{
    BOOL handled = LLToolComposite::handleMouseUp(x, y, mask);
    mIsToolCameraActive = getCurrentTool() == LLToolCamera::getInstance();
    return handled;
}

void LLToolCompInspect::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* hit_obj = pick_info.getObject();
    LLToolCompInspect * tool_inspectp = LLToolCompInspect::getInstance();

    if (!tool_inspectp->mMouseDown)
    {
        // fast click on object, but mouse is already up...just do select
        tool_inspectp->mSelectRect->handleObjectSelection(pick_info, ALControlCache::EditLinkedParts, FALSE);
        return;
    }

    LLSelectMgr * mgr_selectp = LLSelectMgr::getInstance();
    if( hit_obj && mgr_selectp->getSelection()->getObjectCount()) {
        LLEditMenuHandler::gEditMenuHandler = mgr_selectp;
    }

    tool_inspectp->setCurrentTool( tool_inspectp->mSelectRect );
    tool_inspectp->mIsToolCameraActive = FALSE;
    tool_inspectp->mSelectRect->handlePick( pick_info );
}

BOOL LLToolCompInspect::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    return TRUE;
}

BOOL LLToolCompInspect::handleKey(KEY key, MASK mask)
{
    BOOL handled = FALSE;

    if(KEY_ALT == key)
    {
        setCurrentTool(LLToolCamera::getInstance());
        mIsToolCameraActive = TRUE;
        handled = TRUE;
    }
    else
    {
        handled = LLToolComposite::handleKey(key, mask);
    }

    return handled;
}

void LLToolCompInspect::onMouseCaptureLost()
{
    LLToolComposite::onMouseCaptureLost();
    mIsToolCameraActive = FALSE;
}

void LLToolCompInspect::keyUp(KEY key, MASK mask)
{
    if (KEY_ALT == key && mCur == LLToolCamera::getInstance())
    {
        setCurrentTool(mDefault);
        mIsToolCameraActive = FALSE;
    }
}

//----------------------------------------------------------------------------
// LLToolCompTranslate
//----------------------------------------------------------------------------

LLToolCompTranslate::LLToolCompTranslate()
    : LLToolComposite(std::string("Move"))
{
    mManip      = new LLManipTranslate(this);
    mSelectRect     = new LLToolSelectRect(this);

    mCur            = mManip;
    mDefault        = mManip;
}

LLToolCompTranslate::~LLToolCompTranslate()
{
    delete mManip;
    mManip = NULL;

    delete mSelectRect;
    mSelectRect = NULL;
}

BOOL LLToolCompTranslate::handleHover(S32 x, S32 y, MASK mask)
{
    if( !mCur->hasMouseCapture() )
    {
        setCurrentTool( mManip );
    }
    return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompTranslate::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mMouseDown = TRUE;
    gViewerWindow->pickAsync(x, y, mask, pickCallback, /*BOOL pick_transparent*/ FALSE, LLFloaterReg::instanceVisible("build"), FALSE,
        gSavedSettings.getBOOL("SelectReflectionProbes"));;
    return TRUE;
}

void LLToolCompTranslate::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* hit_obj = pick_info.getObject();

    auto tool_comp_translate = LLToolCompTranslate::getInstance();

    LLToolCompTranslate::getInstance()->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
    if (!tool_comp_translate->mMouseDown)
    {
        // fast click on object, but mouse is already up...just do select
        tool_comp_translate->mSelectRect->handleObjectSelection(pick_info, ALControlCache::EditLinkedParts, FALSE);
        return;
    }

    if( hit_obj || tool_comp_translate->mManip->getHighlightedPart() != LLManip::LL_NO_PART )
    {
        if (tool_comp_translate->mManip->getSelection()->getObjectCount())
        {
            LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
        }

        BOOL can_move = tool_comp_translate->mManip->canAffectSelection();

        if( LLManip::LL_NO_PART != tool_comp_translate->mManip->getHighlightedPart() && can_move)
        {
            tool_comp_translate->setCurrentTool( tool_comp_translate->mManip );
            tool_comp_translate->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
        }
        else
        {
            tool_comp_translate->setCurrentTool( tool_comp_translate->mSelectRect );
            tool_comp_translate->mSelectRect->handlePick( pick_info );

            // *TODO: add toggle to trigger old click-drag functionality
            // tool_comp_translate->mManip->handleMouseDownOnPart( XY_part, x, y, mask);
        }
    }
    else
    {
        tool_comp_translate->setCurrentTool( tool_comp_translate->mSelectRect );
        tool_comp_translate->mSelectRect->handlePick( pick_info );
    }
}

BOOL LLToolCompTranslate::handleMouseUp(S32 x, S32 y, MASK mask)
{
    mMouseDown = FALSE;
    return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompTranslate::getOverrideTool(MASK mask)
{
    if (mask == MASK_CONTROL)
    {
        return LLToolCompRotate::getInstance();
    }
    else if (mask == (MASK_CONTROL | MASK_SHIFT))
    {
        return LLToolCompScale::getInstance();
    }
    return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompTranslate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
    {
        // You should already have an object selected from the mousedown.
        // If so, show its properties
        LLFloaterReg::showInstance("build", "Content");
        return TRUE;
    }
    // Nothing selected means the first mouse click was probably
    // bad, so try again.
    // This also consumes the event to prevent things like double-click
    // teleport from triggering.
    return handleMouseDown(x, y, mask);
}


void LLToolCompTranslate::render()
{
    mCur->render(); // removing this will not draw the RGB arrows and guidelines

    if( mCur != mManip )
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        mManip->renderGuidelines();
    }
}


//-----------------------------------------------------------------------
// LLToolCompScale

LLToolCompScale::LLToolCompScale()
    : LLToolComposite(std::string("Stretch"))
{
    mManip = new LLManipScale(this);
    mSelectRect = new LLToolSelectRect(this);

    mCur = mManip;
    mDefault = mManip;
}

LLToolCompScale::~LLToolCompScale()
{
    delete mManip;
    delete mSelectRect;
}

BOOL LLToolCompScale::handleHover(S32 x, S32 y, MASK mask)
{
    if( !mCur->hasMouseCapture() )
    {
        setCurrentTool(mManip );
    }
    return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompScale::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mMouseDown = TRUE;
    gViewerWindow->pickAsync(x, y, mask, pickCallback);
    return TRUE;
}

void LLToolCompScale::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* hit_obj = pick_info.getObject();

    auto tool_comp_scale = LLToolCompScale::getInstance();

    tool_comp_scale->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
    if (!tool_comp_scale->mMouseDown)
    {
        // fast click on object, but mouse is already up...just do select
        tool_comp_scale->mSelectRect->handleObjectSelection(pick_info, ALControlCache::EditLinkedParts, FALSE);

        return;
    }

    if( hit_obj || tool_comp_scale->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
    {
        if (tool_comp_scale->mManip->getSelection()->getObjectCount())
        {
            LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
        }
        if( LLManip::LL_NO_PART != tool_comp_scale->mManip->getHighlightedPart() )
        {
            tool_comp_scale->setCurrentTool( tool_comp_scale->mManip );
            tool_comp_scale->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
        }
        else
        {
            tool_comp_scale->setCurrentTool( tool_comp_scale->mSelectRect );
            tool_comp_scale->mSelectRect->handlePick( pick_info );
        }
    }
    else
    {
        tool_comp_scale->setCurrentTool( tool_comp_scale->mSelectRect );
        tool_comp_scale->mSelectRect->handlePick( pick_info );
    }
}

BOOL LLToolCompScale::handleMouseUp(S32 x, S32 y, MASK mask)
{
    mMouseDown = FALSE;
    return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompScale::getOverrideTool(MASK mask)
{
    if (mask == MASK_CONTROL)
    {
        return LLToolCompRotate::getInstance();
    }

    return LLToolComposite::getOverrideTool(mask);
}


BOOL LLToolCompScale::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
    {
        // You should already have an object selected from the mousedown.
        // If so, show its properties
        LLFloaterReg::showInstance("build", "Content");
        return TRUE;
    }
    else
    {
        // Nothing selected means the first mouse click was probably
        // bad, so try again.
        return handleMouseDown(x, y, mask);
    }
}


void LLToolCompScale::render()
{
    mCur->render();

    if( mCur != mManip )
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        mManip->renderGuidelines();
    }
}

BOOL LLToolCompScale::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
    LLToolCompScale::getInstance()->mManip->handleMiddleMouseDown(x,y,mask);
    return handleMouseDown(x,y,mask);
}

BOOL LLToolCompScale::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
    LLToolCompScale::getInstance()->mManip->handleMiddleMouseUp(x,y,mask);
    return handleMouseUp(x,y,mask);
}

//-----------------------------------------------------------------------
// LLToolCompCreate

LLToolCompCreate::LLToolCompCreate()
    : LLToolComposite(std::string("Create"))
{
    mPlacer = new LLToolPlacer();
    mSelectRect = new LLToolSelectRect(this);

    mCur = mPlacer;
    mDefault = mPlacer;
    mObjectPlacedOnMouseDown = FALSE;
}


LLToolCompCreate::~LLToolCompCreate()
{
    delete mPlacer;
    delete mSelectRect;
}


BOOL LLToolCompCreate::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL handled = FALSE;
    mMouseDown = TRUE;

    if ( (mask == MASK_SHIFT) || (mask == MASK_CONTROL) )
    {
        gViewerWindow->pickAsync(x, y, mask, pickCallback);
        handled = TRUE;
    }
    else
    {
        setCurrentTool( mPlacer );
        handled = mPlacer->placeObject( x, y, mask );
    }

    mObjectPlacedOnMouseDown = TRUE;

    return handled;
}

void LLToolCompCreate::pickCallback(const LLPickInfo& pick_info)
{
    // *NOTE: We mask off shift and control, so you cannot
    // multi-select multiple objects with the create tool.
    MASK mask = (pick_info.mKeyMask & ~MASK_SHIFT);
    mask = (mask & ~MASK_CONTROL);

    auto tool_comp_create = LLToolCompCreate::getInstance();
    tool_comp_create->setCurrentTool(tool_comp_create->mSelectRect );
    tool_comp_create->mSelectRect->handlePick( pick_info );
}

BOOL LLToolCompCreate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    return handleMouseDown(x, y, mask);
}

BOOL LLToolCompCreate::handleMouseUp(S32 x, S32 y, MASK mask)
{
    BOOL handled = FALSE;

    if ( mMouseDown && !mObjectPlacedOnMouseDown && !(mask == MASK_SHIFT) && !(mask == MASK_CONTROL) )
    {
        setCurrentTool( mPlacer );
        handled = mPlacer->placeObject( x, y, mask );
    }

    mObjectPlacedOnMouseDown = FALSE;
    mMouseDown = FALSE;

    if (!handled)
    {
        handled = LLToolComposite::handleMouseUp(x, y, mask);
    }

    return handled;
}

//-----------------------------------------------------------------------
// LLToolCompRotate

LLToolCompRotate::LLToolCompRotate()
    : LLToolComposite(std::string("Rotate"))
{
    mManip = new LLManipRotate(this);
    mSelectRect = new LLToolSelectRect(this);

    mCur = mManip;
    mDefault = mManip;
}


LLToolCompRotate::~LLToolCompRotate()
{
    delete mManip;
    delete mSelectRect;
}

BOOL LLToolCompRotate::handleHover(S32 x, S32 y, MASK mask)
{
    if( !mCur->hasMouseCapture() )
    {
        setCurrentTool( mManip );
    }
    return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mMouseDown = TRUE;
    gViewerWindow->pickAsync(x, y, mask, pickCallback);
    return TRUE;
}

void LLToolCompRotate::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* hit_obj = pick_info.getObject();

    auto tool_comp_rotate = LLToolCompRotate::getInstance();
    tool_comp_rotate->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
    if (!tool_comp_rotate->mMouseDown)
    {
        // fast click on object, but mouse is already up...just do select
        tool_comp_rotate->mSelectRect->handleObjectSelection(pick_info, ALControlCache::EditLinkedParts, FALSE);
        return;
    }

    if( hit_obj || tool_comp_rotate->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
    {
        if (tool_comp_rotate->mManip->getSelection()->getObjectCount())
        {
            LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
        }
        if( LLManip::LL_NO_PART != tool_comp_rotate->mManip->getHighlightedPart() )
        {
            tool_comp_rotate->setCurrentTool( tool_comp_rotate->mManip );
            tool_comp_rotate->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
        }
        else
        {
            tool_comp_rotate->setCurrentTool( tool_comp_rotate->mSelectRect );
            tool_comp_rotate->mSelectRect->handlePick( pick_info );
        }
    }
    else
    {
        tool_comp_rotate->setCurrentTool( tool_comp_rotate->mSelectRect );
        tool_comp_rotate->mSelectRect->handlePick( pick_info );
    }
}

BOOL LLToolCompRotate::handleMouseUp(S32 x, S32 y, MASK mask)
{
    mMouseDown = FALSE;
    return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompRotate::getOverrideTool(MASK mask)
{
    if (mask == (MASK_CONTROL | MASK_SHIFT))
    {
        return LLToolCompScale::getInstance();
    }
    return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompRotate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
    {
        // You should already have an object selected from the mousedown.
        // If so, show its properties
        LLFloaterReg::showInstance("build", "Content");
        return TRUE;
    }
    else
    {
        // Nothing selected means the first mouse click was probably
        // bad, so try again.
        return handleMouseDown(x, y, mask);
    }
}


void LLToolCompRotate::render()
{
    mCur->render();

    if( mCur != mManip )
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        mManip->renderGuidelines();
    }
}


//-----------------------------------------------------------------------
// LLToolCompGun

LLToolCompGun::LLToolCompGun()
    : LLToolComposite(std::string("Mouselook"))
    , mRightMouseDown(false)
    , mTimerFOV()
{
    mGun = new LLToolGun(this);
    mGrab = new LLToolGrabBase(this);
    mNull = sNullTool;

    setCurrentTool(mGun);
    mDefault = mGun;

    mTimerFOV.stop();
    mStartFOV = mOriginalFOV = mTargetFOV = LLViewerCamera::getInstance()->getAndSaveDefaultFOV();
}


LLToolCompGun::~LLToolCompGun()
{
    delete mGun;
    mGun = NULL;

    delete mGrab;
    mGrab = NULL;

    // don't delete a static object
    // delete mNull;
    mNull = NULL;
}

BOOL LLToolCompGun::handleHover(S32 x, S32 y, MASK mask)
{
    // *NOTE: This hack is here to make mouselook kick in again after
    // item selected from context menu.
    if ( mCur == mNull && !gPopupMenuView->getVisible() )
    {
        LLSelectMgr::getInstance()->deselectAll();
        setCurrentTool( (LLTool*) mGrab );
    }

    // Note: if the tool changed, we can't delegate the current mouse event
    // after the change because tools can modify the mouse during selection and deselection.
    // Instead we let the current tool handle the event and then make the change.
    // The new tool will take effect on the next frame.

    mCur->handleHover( x, y, mask );

    // If mouse button not down...
    if( !gViewerWindow->getLeftMouseDown())
    {
        // let ALT switch from gun to grab
        if ( mCur == mGun && (mask & MASK_ALT) )
        {
            setCurrentTool( (LLTool*) mGrab );
        }
        else if ( mCur == mGrab && !(mask & MASK_ALT) )
        {
            setCurrentTool( (LLTool*) mGun );
            setMouseCapture(TRUE);
        }
    }

    return TRUE;
}


BOOL LLToolCompGun::handleMouseDown(S32 x, S32 y, MASK mask)
{
    // if the left button is grabbed, don't put up the pie menu
    if (gAgent.leftButtonGrabbed() && gViewerInput.isLMouseHandlingDefault(MODE_FIRST_PERSON))
    {
        gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
        return FALSE;
    }

    // On mousedown, start grabbing
    gGrabTransientTool = this;
    LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool*) mGrab );

    return LLToolGrab::getInstance()->handleMouseDown(x, y, mask);
}


BOOL LLToolCompGun::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    // if the left button is grabbed, don't put up the pie menu
    if (gAgent.leftButtonGrabbed() && gViewerInput.isLMouseHandlingDefault(MODE_FIRST_PERSON))
    {
        gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
        return FALSE;
    }

    // On mousedown, start grabbing
    gGrabTransientTool = this;
    LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool*) mGrab );

    return LLToolGrab::getInstance()->handleDoubleClick(x, y, mask);
}


BOOL LLToolCompGun::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    if (!(gKeyboard->currentMask(TRUE) & MASK_ALT))
    {
        mRightMouseDown = true;

        if (!mTimerFOV.getStarted())
        {
            mStartFOV = LLViewerCamera::getInstance()->getAndSaveDefaultFOV();
            mOriginalFOV = mStartFOV;
        }
        else
            mStartFOV = LLViewerCamera::getInstance()->getDefaultFOV();

        mTargetFOV = gSavedPerAccountSettings.getF32("AlchemyMouselookAlternativeFOV");
        mTimerFOV.start();

        return TRUE;
    }

    // Returning true will suppress the context menu
    return TRUE;
}

BOOL LLToolCompGun::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
    mRightMouseDown = false;

    mStartFOV = LLViewerCamera::getInstance()->getDefaultFOV();
    mTargetFOV = mOriginalFOV;
    mTimerFOV.start();

    return TRUE;
}

BOOL LLToolCompGun::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (gViewerInput.isLMouseHandlingDefault(MODE_FIRST_PERSON))
    {
        gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_UP);
    }
    setCurrentTool( (LLTool*) mGun );
    return TRUE;
}

void LLToolCompGun::onMouseCaptureLost()
{
    if (mComposite)
    {
        mComposite->onMouseCaptureLost();
        return;
    }
    mCur->onMouseCaptureLost();
}

void    LLToolCompGun::handleSelect()
{
    LLToolComposite::handleSelect();
    setMouseCapture(TRUE);
}

void    LLToolCompGun::handleDeselect()
{
    LLToolComposite::handleDeselect();
    if (mRightMouseDown || mTimerFOV.getStarted())
    {
        LLViewerCamera::getInstance()->loadDefaultFOV();
        mRightMouseDown = false;
        mTimerFOV.stop();
    }
    setMouseCapture(FALSE);
}


BOOL LLToolCompGun::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    if(mRightMouseDown)
    {
        mStartFOV = LLViewerCamera::getInstance()->getDefaultFOV();

        gSavedPerAccountSettings.setF32(
            "AlchemyMouselookAlternativeFOV",
            mTargetFOV = clicks > 0 ?
                llclamp(mTargetFOV += (0.05f * clicks), 0.1f, 3.0f) :
                llclamp(mTargetFOV -= (0.05f * -clicks), 0.1f, 3.0f)
        );

        mTimerFOV.start();
    }
    else if (clicks > 0)
    {
        gAgentCamera.changeCameraToDefault();
    }
    return TRUE;
}

void LLToolCompGun::draw()
{
    if(mTimerFOV.getStarted())
    {
        if(!LLViewerCamera::getInstance()->mSavedFOVLoaded && mStartFOV != mTargetFOV)
        {
            F32 timer = mTimerFOV.getElapsedTimeF32();

            static LLCachedControl<F32> ml_zoom_timeout(gSavedSettings, "AlchemyMouseLookZoomTimeout", 0.15f);
            static LLCachedControl<F32> ml_zoom_time(gSavedSettings, "AlchemyMouseLookZoomTime", 6.66f);
            if(timer > ml_zoom_timeout)
            {
                LLViewerCamera::getInstance()->setDefaultFOV(mTargetFOV);
                mTimerFOV.stop();
            }
            else LLViewerCamera::getInstance()->setDefaultFOV(ll_lerp(mStartFOV, mTargetFOV, timer * ml_zoom_time));
        }
        else mTimerFOV.stop();
    }
    LLToolComposite::draw();
}
