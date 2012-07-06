/*
 *  MiniVstEffectGui.h
 *  MiniVstEffect
 *
 *  Created by hotwatermorning on 12/07/02.
 *  Copyright 2012 home. All rights reserved.
 *
 */
 
#include "vstgui.sf/vstgui/vstgui.h"

namespace hwm {

struct MiniVstEffectEditor
	:	AEffGUIEditor
	,	CControlListener
{
	MiniVstEffectEditor(AudioEffect *eff);
	~MiniVstEffectEditor();
	
	virtual	bool	open(void *ptr);
	virtual void	close();
	virtual void	setParameter(VstInt32 index, float value);
	virtual void	valueChanged(CDrawContext *context, CControl *control);	
	virtual void	idle();
	
private:
	CSlider	*freq_;
	CSlider *gain_;
	CSlider *Q_;
	CSlider *filter_type_;
	
	CTextLabel *title_freq_;
	CTextLabel *title_gain_;
	CTextLabel *title_Q_;
	CTextLabel *title_filter_type_;

	CTextLabel *disp_freq_;
	CTextLabel *disp_gain_;
	CTextLabel *disp_Q_;
	CTextLabel *disp_filter_type_;
	
	CBitmap *background_;
};

}	//namespace hwm