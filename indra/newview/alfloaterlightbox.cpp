/**
 * @file alfloaterlightbox.cpp
 * @brief A generic text floater for dumping info (usually debug info)
 *
 * Copyright (c) 2022, Rye Mutt <rye@alchemyviewer.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "llviewerprecompiledheaders.h"
#include "alfloaterlightbox.h"

#include "alrenderutils.h"
#include "llviewercontrol.h"
#include "llspinctrl.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "llcombobox.h"

ALFloaterLightBox::ALFloaterLightBox(const LLSD& key)
:	LLFloater(key)
{
    mCommitCallbackRegistrar.add("LightBox.ResetControlDefault", std::bind(&ALFloaterLightBox::onClickResetControlDefault, this, std::placeholders::_2));
    mCommitCallbackRegistrar.add("LightBox.ResetGroupDefault", std::bind(&ALFloaterLightBox::onClickResetGroupDefault, this, std::placeholders::_2));
}

ALFloaterLightBox::~ALFloaterLightBox()
{
	mTonemapConnection.disconnect();
	mCASConnection.disconnect();
}

BOOL ALFloaterLightBox::postBuild()
{
	updateTonemapper();
	mTonemapConnection = gSavedSettings.getControl("RenderToneMapType")->getSignal()->connect([&](LLControlVariable* control, const LLSD&, const LLSD&){ updateTonemapper(); });
	mCASConnection = gSavedSettings.getControl("RenderSharpenMethod")->getSignal()->connect([&](LLControlVariable* control, const LLSD&, const LLSD&){ updateCAS(); });
	populateLUTCombo();
	return TRUE;
}

void ALFloaterLightBox::draw()
{
    updateCAS();
    LLFloater::draw();
}

void ALFloaterLightBox::populateLUTCombo()
{
	LLComboBox* lut_combo = getChild<LLComboBox>("colorlut_combo");
	const std::string& user_luts = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "colorlut");
	
#if LL_WINDOWS
	boost::filesystem::path user_luts_path(ll_convert_string_to_wide(user_luts));
#else
	boost::filesystem::path user_luts_path(user_luts);
#endif
	
	if(boost::filesystem::is_directory(user_luts_path))
	{
		if(!boost::filesystem::is_empty(user_luts_path))
		{
			lut_combo->addSeparator();
		}
		for (boost::filesystem::directory_entry& lut : boost::filesystem::directory_iterator(user_luts_path))
		{
			std::string lut_stem = lut.path().stem().string();
			std::string lut_filename = lut.path().filename().string();
			lut_combo->add(lut_stem, lut_filename);
		}
		lut_combo->selectByValue(gSavedSettings.getString("RenderColorGradeLUT"));
		lut_combo->resetDirty();
	}
}

void ALFloaterLightBox::onClickResetControlDefault(const LLSD& userdata)
{
	const std::string& control_name = userdata.asString();
	LLControlVariable* controlp = gSavedSettings.getControl(control_name);
	if (controlp)
	{
		controlp->resetToDefault(true);
	}
}

void ALFloaterLightBox::onClickResetGroupDefault(const LLSD& userdata)
{
	const std::string& setting_group = userdata.asString();
	if (setting_group == "sharpen")
	{
		LLControlVariable* controlp = gSavedSettings.getControl("RenderSharpenMethod");
		if (controlp)
		{
			controlp->resetToDefault(true);
		}
		controlp = gSavedSettings.getControl("RenderSharpenCASSharpness");
		if (controlp)
		{
			controlp->resetToDefault(true);
		}
		controlp = gSavedSettings.getControl("RenderSharpenDLSSharpness");
		if (controlp)
		{
			controlp->resetToDefault(true);
		}
		controlp = gSavedSettings.getControl("RenderSharpenDLSDenoise");
		if (controlp)
		{
			controlp->resetToDefault(true);
		}
	}
	else if (setting_group == "tonemap")
	{
		{
			LLControlVariable* controlp = gSavedSettings.getControl("RenderExposure");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
		}

		U32 tone_map_type = gSavedSettings.getU32("RenderToneMapType");
		switch (tone_map_type)
		{
		case ALRenderUtil::TONEMAP_AMD:
		{
			LLControlVariable* controlp = gSavedSettings.getControl("AlchemyToneMapAMDHDRMax");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapAMDExposure");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapAMDContrast");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapAMDSaturationR");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapAMDSaturationG");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapAMDSaturationB");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			break;
		}
		case ALRenderUtil::TONEMAP_UCHIMURA:
		{
			LLControlVariable* controlp = gSavedSettings.getControl("AlchemyToneMapUchimuraMaxBrightness");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapUchimuraContrast");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapUchimuraLinearStart");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapUchimuraLinearLength");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapUchimuraBlackLevel");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			break;
		}
		case ALRenderUtil::TONEMAP_UNCHARTED:
		{
			LLControlVariable* controlp = gSavedSettings.getControl("AlchemyToneMapFilmicToeStr");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicToeLen");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicShoulderStr");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicShoulderLen");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicShoulderAngle");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicGamma");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			controlp = gSavedSettings.getControl("AlchemyToneMapFilmicWhitePoint");
			if (controlp)
			{
				controlp->resetToDefault(true);
			}
			break;
		}
		}
	}
}

void ALFloaterLightBox::updateTonemapper()
{
	//Init Text
	LLTextBox* text1 = getChild<LLTextBox>("tonemapper_dynamic_text1");
	LLTextBox* text2 = getChild<LLTextBox>("tonemapper_dynamic_text2");
	LLTextBox* text3 = getChild<LLTextBox>("tonemapper_dynamic_text3");
	LLTextBox* text4 = getChild<LLTextBox>("tonemapper_dynamic_text4");
	LLTextBox* text5 = getChild<LLTextBox>("tonemapper_dynamic_text5");
	LLTextBox* text6 = getChild<LLTextBox>("tonemapper_dynamic_text6");
	LLTextBox* text7 = getChild<LLTextBox>("tonemapper_dynamic_text7");

	//Init Spinners
	LLSpinCtrl* spinner1 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner1");
	LLSpinCtrl* spinner2 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner2");
	LLSpinCtrl* spinner3 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner3");
	LLSpinCtrl* spinner4 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner4");
	LLSpinCtrl* spinner5 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner5");
	LLSpinCtrl* spinner6 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner6");
	LLSpinCtrl* spinner7 = getChild<LLSpinCtrl>("tonemapper_dynamic_spinner7");

	// Init Sliders
	LLSliderCtrl* slider1 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider1");
	LLSliderCtrl* slider2 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider2");
	LLSliderCtrl* slider3 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider3");
	LLSliderCtrl* slider4 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider4");
	LLSliderCtrl* slider5 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider5");
	LLSliderCtrl* slider6 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider6");
	LLSliderCtrl* slider7 = getChild<LLSliderCtrl>("tonemapper_dynamic_slider7");

	// Check the state of RenderToneMapType
	switch (gSavedSettings.getU32("RenderToneMapType"))
	{
	default:
	{
		text1->setVisible(FALSE);
		spinner1->setVisible(FALSE);
		slider1->setVisible(FALSE);

		text2->setVisible(FALSE);
		spinner2->setVisible(FALSE);
		slider2->setVisible(FALSE);

		text3->setVisible(FALSE);
		spinner3->setVisible(FALSE);
		slider3->setVisible(FALSE);

		text4->setVisible(FALSE);
		spinner4->setVisible(FALSE);
		slider4->setVisible(FALSE);

		text5->setVisible(FALSE);
		spinner5->setVisible(FALSE);
		slider5->setVisible(FALSE);

		text6->setVisible(FALSE);
		spinner6->setVisible(FALSE);
		slider6->setVisible(FALSE);

		text7->setVisible(FALSE);
		spinner7->setVisible(FALSE);
		slider7->setVisible(FALSE);
		break;
	}
	case ALRenderUtil::TONEMAP_UCHIMURA:
	{
		text1->setVisible(TRUE);
		text1->setText(std::string("Max Brightness"));
		spinner1->setVisible(TRUE);
		spinner1->setMinValue(0.01);
		spinner1->setMaxValue(8.0);
		spinner1->setIncrement(0.1);
		spinner1->setControlName("AlchemyToneMapUchimuraMaxBrightness");
		slider1->setVisible(TRUE);
		slider1->setMinValue(0.01);
		slider1->setMaxValue(8.0);
		slider1->setIncrement(0.1);
		slider1->setControlName("AlchemyToneMapUchimuraMaxBrightness", nullptr);

		text2->setVisible(TRUE);
		text2->setText(std::string("Contrast"));
		spinner2->setVisible(TRUE);
		spinner2->setMinValue(0.01);
		spinner2->setMaxValue(2.0);
		spinner2->setIncrement(0.01);
		spinner2->setControlName("AlchemyToneMapUchimuraContrast");
		slider2->setVisible(TRUE);
		slider2->setMinValue(0.01);
		slider2->setMaxValue(2.0);
		slider2->setIncrement(0.01);
		slider2->setControlName("AlchemyToneMapUchimuraContrast", nullptr);

		text3->setVisible(TRUE);
		text3->setText(std::string("Linear Start"));
		spinner3->setVisible(TRUE);
		spinner3->setMinValue(0.01);
		spinner3->setMaxValue(1.0);
		spinner3->setIncrement(0.01);
		spinner3->setControlName("AlchemyToneMapUchimuraLinearStart");
		slider3->setVisible(TRUE);
		slider3->setMinValue(0.01);
		slider3->setMaxValue(1.0);
		slider3->setIncrement(0.01);
		slider3->setControlName("AlchemyToneMapUchimuraLinearStart", nullptr);

		text4->setVisible(TRUE);
		text4->setText(std::string("Linear Length"));
		spinner4->setVisible(TRUE);
		spinner4->setMinValue(0.01);
		spinner4->setMaxValue(1.0);
		spinner4->setIncrement(0.01);
		spinner4->setControlName("AlchemyToneMapUchimuraLinearLength");
		slider4->setVisible(TRUE);
		slider4->setMinValue(0.01);
		slider4->setMaxValue(1.0);
		slider4->setIncrement(0.01);
		slider4->setControlName("AlchemyToneMapUchimuraLinearLength", nullptr);

		text5->setVisible(TRUE);
		text5->setText(std::string("Black Level"));
		spinner5->setVisible(TRUE);
		spinner5->setMinValue(0.01);
		spinner5->setMaxValue(4.0);
		spinner5->setIncrement(0.01);
		spinner5->setControlName("AlchemyToneMapUchimuraBlackLevel");
		slider5->setVisible(TRUE);
		slider5->setMinValue(0.01);
		slider5->setMaxValue(4.0);
		slider5->setIncrement(0.01);
		slider5->setControlName("AlchemyToneMapUchimuraBlackLevel", nullptr);

		text6->setVisible(FALSE);
		spinner6->setVisible(FALSE);
		slider6->setVisible(FALSE);

		text7->setVisible(FALSE);
		spinner7->setVisible(FALSE);
		slider7->setVisible(FALSE);
		break;
	}
	case ALRenderUtil::TONEMAP_AMD:
	{
		text1->setVisible(TRUE);
		text1->setText(std::string("HDR Max"));
		spinner1->setVisible(TRUE);
		spinner1->setMinValue(1.0);
		spinner1->setMaxValue(512.0);
		spinner1->setIncrement(1.f);
		spinner1->setControlName("AlchemyToneMapAMDHDRMax");
		slider1->setVisible(TRUE);
		slider1->setMinValue(1.0);
		slider1->setMaxValue(512.0);
		slider1->setIncrement(1.f);
		slider1->setControlName("AlchemyToneMapAMDHDRMax", nullptr);

		text2->setVisible(TRUE);
		text2->setText(std::string("Tone Exposure"));
		spinner2->setVisible(TRUE);
		spinner2->setMinValue(1.0);
		spinner2->setMaxValue(16.0);
		spinner2->setIncrement(0.1);
		spinner2->setControlName("AlchemyToneMapAMDExposure");
		slider2->setVisible(TRUE);
		slider2->setMinValue(1.0);
		slider2->setMaxValue(16.0);
		slider2->setIncrement(0.1);
		slider2->setControlName("AlchemyToneMapAMDExposure", nullptr);

		text3->setVisible(TRUE);
		text3->setText(std::string("Contrast"));
		spinner3->setVisible(TRUE);
		spinner3->setMinValue(0.0);
		spinner3->setMaxValue(1.0);
		spinner3->setIncrement(0.01);
		spinner3->setControlName("AlchemyToneMapAMDContrast");
		slider3->setVisible(TRUE);
		slider3->setMinValue(0.0);
		slider3->setMaxValue(1.0);
		slider3->setIncrement(0.01);
		slider3->setControlName("AlchemyToneMapAMDContrast", nullptr);

		text4->setVisible(TRUE);
		text4->setText(std::string("R Saturation"));
		spinner4->setVisible(TRUE);
		spinner4->setMinValue(-2.0);
		spinner4->setMaxValue(2.0);
		spinner4->setIncrement(0.1);
		spinner4->setControlName("AlchemyToneMapAMDSaturationR");
		slider4->setVisible(TRUE);
		slider4->setMinValue(-2.0);
		slider4->setMaxValue(2.0);
		slider4->setIncrement(0.1);
		slider4->setControlName("AlchemyToneMapAMDSaturationR", nullptr);

		text5->setVisible(TRUE);
		text5->setText(std::string("G Saturation"));
		spinner5->setVisible(TRUE);
		spinner5->setMinValue(-2.0);
		spinner5->setMaxValue(2.0);
		spinner5->setIncrement(0.1);
		spinner5->setControlName("AlchemyToneMapAMDSaturationG");
		slider5->setVisible(TRUE);
		slider5->setMinValue(-2.0);
		slider5->setMaxValue(2.0);
		slider5->setIncrement(0.1);
		slider5->setControlName("AlchemyToneMapAMDSaturationG", nullptr);

		text6->setVisible(TRUE);
		text6->setText(std::string("B Saturation"));
		spinner6->setVisible(TRUE);
		spinner6->setMinValue(-2.0);
		spinner6->setMaxValue(2.0);
		spinner6->setIncrement(0.1);
		spinner6->setControlName("AlchemyToneMapAMDSaturationB");
		slider6->setVisible(TRUE);
		slider6->setMinValue(-2.0);
		slider6->setMaxValue(2.0);
		slider6->setIncrement(0.1);
		slider6->setControlName("AlchemyToneMapAMDSaturationB", nullptr);

		text7->setVisible(FALSE);
		spinner7->setVisible(FALSE);
		slider7->setVisible(FALSE);
		break;
	}
	case ALRenderUtil::TONEMAP_UNCHARTED:
	{
		text1->setVisible(TRUE);
		text1->setText(std::string("Toe Strength"));
		spinner1->setVisible(TRUE);
		spinner1->setMinValue(0.0);
		spinner1->setMaxValue(1.0);
		spinner1->setIncrement(0.01);
		spinner1->setControlName("AlchemyToneMapFilmicToeStr");
		slider1->setVisible(TRUE);
		slider1->setMinValue(0.0);
		slider1->setMaxValue(1.0);
		slider1->setIncrement(0.01);
		slider1->setControlName("AlchemyToneMapFilmicToeStr", nullptr);

		text2->setVisible(TRUE);
		text2->setText(std::string("Toe Length"));
		spinner2->setVisible(TRUE);
		spinner2->setMinValue(0.01);
		spinner2->setMaxValue(1.0);
		spinner2->setIncrement(0.01);
		spinner2->setControlName("AlchemyToneMapFilmicToeLen");
		slider2->setVisible(TRUE);
		slider2->setMinValue(0.01);
		slider2->setMaxValue(1.0);
		slider2->setIncrement(0.01);
		slider2->setControlName("AlchemyToneMapFilmicToeLen", nullptr);

		text3->setVisible(TRUE);
		text3->setText(std::string("Shoulder Strength"));
		spinner3->setVisible(TRUE);
		spinner3->setMinValue(0.0);
		spinner3->setMaxValue(1.0);
		spinner3->setIncrement(0.01);
		spinner3->setControlName("AlchemyToneMapFilmicShoulderStr");
		slider3->setVisible(TRUE);
		slider3->setMinValue(0.0);
		slider3->setMaxValue(1.0);
		slider3->setIncrement(0.01);
		slider3->setControlName("AlchemyToneMapFilmicShoulderStr", nullptr);

		text4->setVisible(TRUE);
		text4->setText(std::string("Shoulder Length"));
		spinner4->setVisible(TRUE);
		spinner4->setMinValue(0.01);
		spinner4->setMaxValue(8.0);
		spinner4->setIncrement(0.01);
		spinner4->setControlName("AlchemyToneMapFilmicShoulderLen");
		slider4->setVisible(TRUE);
		slider4->setMinValue(0.01);
		slider4->setMaxValue(8.0);
		slider4->setIncrement(0.01);
		slider4->setControlName("AlchemyToneMapFilmicShoulderLen", nullptr);

		text5->setVisible(TRUE);
		text5->setText(std::string("Shoulder Angle"));
		spinner5->setVisible(TRUE);
		spinner5->setMinValue(0.0);
		spinner5->setMaxValue(1.0);
		spinner5->setIncrement(0.01);
		spinner5->setControlName("AlchemyToneMapFilmicShoulderAngle");
		slider5->setVisible(TRUE);
		slider5->setMinValue(0.0);
		slider5->setMaxValue(1.0);
		slider5->setIncrement(0.01);
		slider5->setControlName("AlchemyToneMapFilmicShoulderAngle", nullptr);

		text6->setVisible(TRUE);
		text6->setText(std::string("Gamma"));
		spinner6->setVisible(TRUE);
		spinner6->setMinValue(0.01);
		spinner6->setMaxValue(5.0);
		spinner6->setIncrement(0.01);
		spinner6->setControlName("AlchemyToneMapFilmicGamma");
		slider6->setVisible(TRUE);
		slider6->setMinValue(0.01);
		slider6->setMaxValue(5.0);
		slider6->setIncrement(0.01);
		slider6->setControlName("AlchemyToneMapFilmicGamma", nullptr);

		text7->setVisible(TRUE);
		text7->setText(std::string("White Point"));
		spinner7->setVisible(TRUE);
		spinner7->setMinValue(1.0);
		spinner7->setMaxValue(16.0);
		spinner7->setIncrement(0.1);
		spinner7->setControlName("AlchemyToneMapFilmicWhitePoint");
		slider7->setVisible(TRUE);
		slider7->setMinValue(1.0);
		slider7->setMaxValue(16.0);
		slider7->setIncrement(0.1);
		slider7->setControlName("AlchemyToneMapFilmicWhitePoint", nullptr);
		break;
	}
	}
}

void ALFloaterLightBox::updateCAS()
{
	// Init UI
	LLTextBox* text2 = getChild<LLTextBox>("sharp_dynamic_text");
	LLSpinCtrl* spinner1 = getChild<LLSpinCtrl>("sharp_strength_spinner");
	LLSpinCtrl* spinner2 = getChild<LLSpinCtrl>("sharp_dynamic_spinner");
	LLSliderCtrl* slider1 = getChild<LLSliderCtrl>("sharp_strength_slider");
	LLSliderCtrl* slider2 = getChild<LLSliderCtrl>("sharp_dynamic_slider");

	switch (gSavedSettings.getU32("RenderSharpenMethod"))
	{
	default:
	case ALRenderUtil::SHARPEN_NONE:
	{
		spinner1->setVisible(FALSE);
		slider1->setVisible(FALSE);
		text2->setVisible(FALSE);
		spinner2->setVisible(FALSE);
		slider2->setVisible(FALSE);
		break;
	}
	case ALRenderUtil::SHARPEN_CAS:
	{
		spinner1->setVisible(TRUE);
		spinner1->setMinValue(0.0);
		spinner1->setMaxValue(1.0);
		spinner1->setIncrement(0.1);
		spinner1->setControlName("RenderSharpenCASSharpness");
		slider1->setVisible(TRUE);
		slider1->setMinValue(0.0);
		slider1->setMaxValue(1.0);
		slider1->setIncrement(0.1);
		slider1->setControlName("RenderSharpenCASSharpness", nullptr);

		text2->setVisible(FALSE);
		spinner2->setVisible(FALSE);
		slider2->setVisible(FALSE);
		break;
	}
	case ALRenderUtil::SHARPEN_DLS:
	{
		spinner1->setVisible(TRUE);
		spinner1->setMinValue(0.0);
		spinner1->setMaxValue(1.0);
		spinner1->setIncrement(0.1);
		spinner1->setControlName("RenderSharpenDLSSharpness");
		slider1->setVisible(TRUE);
		slider1->setMinValue(0.0);
		slider1->setMaxValue(1.0);
		slider1->setIncrement(0.1);
		slider1->setControlName("RenderSharpenDLSSharpness", nullptr);

		text2->setVisible(TRUE);
		text2->setText(std::string("Denoise:"));
		spinner2->setVisible(TRUE);
		spinner2->setMinValue(0.0);
		spinner2->setMaxValue(1.0);
		spinner2->setIncrement(0.1);
		spinner2->setControlName("RenderSharpenDLSDenoise");
		slider2->setVisible(TRUE);
		slider2->setMinValue(0.0);
		slider2->setMaxValue(1.0);
		slider2->setIncrement(0.1);
		slider2->setControlName("RenderSharpenDLSDenoise", nullptr);
		break;
	}
	}
}
