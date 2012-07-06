/*
 *  MiniVstEffectGui.cpp
 *  MiniVstEffect
 *
 *  Created by hotwatermorning on 12/07/02.
 *  Copyright 2012 home. All rights reserved.
 *
 */

#include <string>
#include "MiniVstEffectEditor.h"
#include "MiniVstEffect.hpp"

namespace hwm {

namespace {

struct defines {

	enum {
		kBackgroundId = 101,
		kSliderBack,
		kSliderHandle
	};
};

}	//unnamed namespace

typedef hwm::MiniVstEffect MiniVstEffect;

MiniVstEffectEditor::MiniVstEffectEditor(AudioEffect *eff)
:	AEffGUIEditor(eff)
,	freq_(0)
,	gain_(0)
,	Q_(0)
,	filter_type_(0)
,	disp_freq_(0)
,	disp_gain_(0)
,	disp_Q_(0)
,	disp_filter_type_(0)
,	background_(0)
{
	background_ = new CBitmap(defines::kBackgroundId);
	
	rect.left = 0;
	rect.top = 0;
	rect.right = static_cast<short>(background_->getWidth());
	rect.bottom = static_cast<short>(background_->getHeight());
}

MiniVstEffectEditor::~MiniVstEffectEditor()
{
	if(background_) {
		background_->forget();
		background_ = 0;
	}
}

bool MiniVstEffectEditor::open(void *ptr)
{
	AEffGUIEditor::open(ptr);
	
	CBitmap *bm_slider_back = new CBitmap(defines::kSliderBack);
	CBitmap *bm_slider_handle = new CBitmap(defines::kSliderHandle);

	size_t const shift_width = bm_slider_back->getWidth() * 2;
	
	CRect frame_size(
		0, 0, background_->getWidth(), background_->getHeight()
		);
	
	CFrame *frm = new CFrame(frame_size, ptr, this);
	
	frm->setBackground(background_);
	
	CRect slider_rect(
		20,
		40,
		20 + bm_slider_back->getWidth(),
		40 + bm_slider_back->getHeight()
		);

	CRect title_rect(
		slider_rect.left - 13,
		10,
		slider_rect.right + 13,
		30 );
		
	CRect disp_rect(
		slider_rect.left - 13,
		slider_rect.bottom + 10,
		slider_rect.right + 13,
		slider_rect.bottom + 30);

	int const min_pos = slider_rect.top + bm_slider_handle->getHeight() / 2;
	int const max_pos = slider_rect.top + bm_slider_back->getHeight() - bm_slider_handle->getHeight();
	
	CPoint handle_point(5, 0);
	CPoint offset_bg(0, 1);
	
	//! Frequency
	title_freq_ = new CTextLabel(title_rect);
	title_freq_->setFont(kNormalFontVerySmall);
	title_freq_->setText("Frequency");

	freq_ = new CVerticalSlider(
		slider_rect, this, MiniVstEffect::kCutOff,
		min_pos, max_pos,
		bm_slider_handle, bm_slider_back,
		offset_bg );
	freq_->setOffsetHandle(handle_point);
	freq_->setDirty();
	
	disp_freq_ = new CTextLabel(disp_rect);
	disp_freq_->setFont(kNormalFontVerySmall);
	
	frm->addView(title_freq_);
	frm->addView(freq_);
	frm->addView(disp_freq_);

	title_rect.offset(shift_width, 0);
	slider_rect.offset(shift_width, 0);
	disp_rect.offset(shift_width, 0);

	//! Gain
	title_gain_ = new CTextLabel(title_rect);
	title_gain_->setFont(kNormalFontVerySmall);
	title_gain_->setText("Eq Gain");
		
	gain_ = new CVerticalSlider(
		slider_rect, this, MiniVstEffect::kdBGain,
		min_pos, max_pos,
		bm_slider_handle, bm_slider_back,
		offset_bg );
	gain_->setOffsetHandle(handle_point);
	gain_->setDirty();
	
	disp_gain_ = new CTextLabel(disp_rect);
	disp_gain_->setFont(kNormalFontVerySmall);
	
	frm->addView(title_gain_);
	frm->addView(gain_);
	frm->addView(disp_gain_);
	
	title_rect.offset(shift_width, 0);
	slider_rect.offset(shift_width, 0);
	disp_rect.offset(shift_width, 0);

	//! Q
	title_Q_ = new CTextLabel(title_rect);
	title_Q_->setFont(kNormalFontVerySmall);
	title_Q_->setText("Q");
		
	Q_ = new CVerticalSlider(
		slider_rect, this, MiniVstEffect::kQ,
		min_pos, max_pos,
		bm_slider_handle, bm_slider_back,
		offset_bg );
	Q_->setOffsetHandle(handle_point);
	Q_->setDirty();
	
	disp_Q_ = new CTextLabel(disp_rect);
	disp_Q_->setFont(kNormalFontVerySmall);
	
	frm->addView(title_Q_);
	frm->addView(Q_);
	frm->addView(disp_Q_);

	title_rect.offset(shift_width, 0);
	slider_rect.offset(shift_width, 0);
	disp_rect.offset(shift_width, 0);

	//! filter
	title_filter_type_ = new CTextLabel(title_rect);
	title_filter_type_->setFont(kNormalFontVerySmall);
	title_filter_type_->setText("Filter");
		
	filter_type_ = new CVerticalSlider(
		slider_rect, this, MiniVstEffect::kFilterType,
		min_pos, max_pos,
		bm_slider_handle, bm_slider_back,
		offset_bg );
	filter_type_->setOffsetHandle(handle_point);
	filter_type_->setDirty();
	
	disp_filter_type_ = new CTextLabel(disp_rect);
	disp_filter_type_->setFont(kNormalFontVerySmall);
	
	frm->addView(title_filter_type_);
	frm->addView(filter_type_);
	frm->addView(disp_filter_type_);
	
	bm_slider_back->forget();
	bm_slider_handle->forget();
	
	frame = frm;
	return true;
}

void MiniVstEffectEditor::close()
{
	delete frame;
	frame = 0;
}

void MiniVstEffectEditor::setParameter(VstInt32 index, float value)
{
	std::string const sp = " ";
	
	char disp_buf[kVstMaxParamStrLen+1] = {};
	char label_buf[kVstMaxParamStrLen+1] = {};
				
	switch (index) {
		case MiniVstEffect::kCutOff:
			if(freq_) {
				freq_->setValue(effect->getParameter(index));
			}
			if(disp_freq_) {
				effect->getParameterDisplay(index, disp_buf);
				effect->getParameterLabel(index, label_buf);
				disp_freq_->setText((disp_buf + sp + label_buf).c_str());
			}
			break;
		
		case MiniVstEffect::kdBGain:
			if(gain_) {
				gain_->setValue(effect->getParameter(index));
			}
			if(disp_gain_) {
				effect->getParameterDisplay(index, disp_buf);
				effect->getParameterLabel(index, label_buf);
				disp_gain_->setText((disp_buf + sp + label_buf).c_str());
			}
			break;
			
		case MiniVstEffect::kQ:
			if(Q_) {
				Q_->setValue(effect->getParameter(index));
			}
			if(disp_Q_) {
				effect->getParameterDisplay(index, disp_buf);
				effect->getParameterLabel(index, label_buf);
				disp_Q_->setText((disp_buf + sp + label_buf).c_str());
			}
			break;
			
		case MiniVstEffect::kFilterType:
			if(filter_type_) {
				filter_type_->setValue(effect->getParameter(index));
			}
			if(disp_filter_type_) {
				effect->getParameterDisplay(index, disp_buf);
				effect->getParameterLabel(index, label_buf);
				disp_filter_type_->setText((disp_buf + sp + label_buf).c_str());
			}
			break;
			
		default:
			break;
	}
}

void MiniVstEffectEditor::valueChanged(CDrawContext *context, CControl *control)
{
	long tag = control->getTag ();
	switch (tag)
	{
		case MiniVstEffect::kCutOff:
		case MiniVstEffect::kdBGain:
		case MiniVstEffect::kQ:
		case MiniVstEffect::kFilterType:
			effect->setParameterAutomated (tag, control->getValue ());
			setParameter(tag, control->getValue());
			control->setDirty ();
		break;
	}
}

void MiniVstEffectEditor::idle()
{
	frame->redraw();
}

}	//namespace hwm