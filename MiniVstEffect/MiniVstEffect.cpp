#include "./MiniVstEffect.hpp"
#define _USE_MATH_DEFINES

#include <cmath>
#include <cassert>
#include <sstream>
#include <vector>
#include "./MiniVstEffecteditor.h"

namespace hwm {

struct defines{

	//! parameter index
	enum {
		kNumPrograms	= 8,
		kVendorVersion	= 1
	};

	static int const	kID;
	static char const *kVendor;
	static char const *kProduct;
	static char const *kEffect;

	static char const *kProgNames[kNumPrograms];

	//! フィルタタイプの定義
	enum {
		LPF,
		HPF,
		BPF,
		notch,
		APF,
		PeakingEQ,
		LowShelf,
		HighShelf,
		kNumFilterType
	};

	static VstProgram const presets[defines::kNumPrograms];

	static double const kdBMin;
	static double const kdBMax;
	static double const kdBRange;
	
	
	//! vstのパラメータ値をfilterのインデックスに
	static
	size_t	param_to_filter(vst_param_t value)
	{
		return
			static_cast<size_t>(value * kNumFilterType - 0.5);
	}
	
	//! filterのインデックスをvstのパラメータ値に
	static
	vst_param_t
			filter_to_param(size_t filter)
	{
		return
			static_cast<vst_param_t>((filter + 0.5) / kNumFilterType);
	}
	
	//! パラメータとdB
	static
	double	param_to_db(vst_param_t value)
	{
		return 20.0 * log10(static_cast<double>(value * 4.0));
	}

	//! パラメータとdB
	static
	vst_param_t
			db_to_param(double dB)
	{
		return
			static_cast<vst_param_t>(pow(10.0, (dB)/20.0) / 4.0);
	}

	//! フィルタタイプから、そのフィルタを表す文字列を取得
	static
	char const *
			get_filter_string(size_t filter_type)
	{
		switch(filter_type) {
			case defines::LPF:
				return "LPF";
			case defines::HPF:
				return "HPF";
			case defines::BPF:
				return "BPF";
			case defines::notch:
				return "notch";
			case defines::APF:
				return "APF";
			case defines::PeakingEQ:
				return "PeakEQ";
			case defines::LowShelf:
				return "Lo-Shelf";
			case defines::HighShelf:
				return "Hi-Shelf";
		}
		return "Unknown";
	}
};

int	const defines::kID					= 'MVFx';
char const *	defines::kVendor		= "hotwatermorning";
char const *	defines::kProduct		= "Mini Vst Eq";
char const *	defines::kEffect		= defines::kProduct;

//! プラグインのプリセット
VstProgram	const 
				defines::presets[defines::kNumPrograms] = {
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::LPF), "Low Pass Filter"},
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::HPF), "High Pass Filter"},
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::BPF), "Band Pass Filter"},
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::notch), "notch Filter"},
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::APF), "All-Pass Filter"},
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::PeakingEQ), "peaking EQ" },
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::LowShelf), "Low Shelving Filter" },
	{ 0.5, 0.0, 0.0, defines::filter_to_param(defines::HighShelf), "High Shelving Filter" }
};

double const	defines::kdBMin			= -100.0;
double const	defines::kdBMax			= 20.0;
double const	defines::kdBRange		= defines::kdBMax - defines::kdBMin;

//! MiniVstEffectの実装
MiniVstEffect::MiniVstEffect(audioMasterCallback audioMaster)
	:	AudioEffectX(
			audioMaster,
			defines::kNumPrograms,
			kNumParams )
	,	b0_(0.0)
	,	b1_(0.0)
	,	b2_(0.0)
	,	a0_(0.0)
	,	a1_(0.0)
	,	a2_(0.0)
{	
	//! 出入力チャンネルの設定
	setNumInputs(kNumChannels);
	setNumOutputs(kNumChannels);

	//! processReplacingが使用できることを表明
	canProcessReplacing();
	
	//! processDoubleReplacingが使用できることを表明
	canDoubleReplacing();
	
	//! uniqueIDの設定
	setUniqueID(defines::kID);
	
	//! Editorの設定
	editor = new MiniVstEffectEditor(this);

	//! 先頭のプログラムを設定しておく
	setProgram(0);
}

MiniVstEffect::~MiniVstEffect()
{}

void	MiniVstEffect::setProgram		(VstInt32 program)
{
	cur_program_ = defines::presets[program];
	reset_coeffs();
	clear_buffer();
}

void	MiniVstEffect::setProgramName	(char *name)
{
	get_current_program().name_ = name;
}

void	MiniVstEffect::getProgramName	(char *name)
{
	vst_strncpy(name, cur_program_.name_.c_str(), kVstMaxProgNameLen);
}

bool	MiniVstEffect::getProgramNameIndexed (VstInt32 /*unused*/, VstInt32 index, char* text)
{
	vst_strncpy(text, defines::presets[index].name_.c_str(), kVstMaxProgNameLen);
	return true;
}

void	MiniVstEffect::setParameter		(VstInt32 index, vst_param_t value)
{
	AudioEffectX::setParameter(index, value);
	bool filter_changed = false;
	
	switch(index) {
	case kCutOff:
		get_current_program().cutoff_ = value;
		break;

	case kdBGain:
		get_current_program().db_gain_ = value;
		break;

	case kQ:
		get_current_program().Q_ = value;
		break;

	case kFilterType:
		if( defines::param_to_filter(get_current_program().filter_type_) !=
			defines::param_to_filter(value) )
		{
			filter_changed = true;
		}
		get_current_program().filter_type_ = value;
		break;
	}

	this->reset_coeffs();
	
	if(filter_changed) {
		clear_buffer();
	}
	if (editor) {
		((AEffGUIEditor*)editor)->setParameter (index, value);
	}
}

vst_param_t
		MiniVstEffect::getParameter		(VstInt32 index)
{
	switch(index) {
	case kCutOff:
		return get_current_program().cutoff_;

	case kdBGain:
		return get_current_program().db_gain_;

	case kQ:
		return get_current_program().Q_;

	case kFilterType:
		return get_current_program().filter_type_;
	}
	
	return 0;
}

void	MiniVstEffect::getParameterName(VstInt32 index, char *label)
{
	switch(index) {
		case kCutOff:
			vst_strncpy(label, "Cutoff", kVstMaxParamStrLen);
			break;

		case kdBGain:
			vst_strncpy(label, "dB Gain", kVstMaxParamStrLen);
			break;

		case kQ:
			vst_strncpy(label, "Q", kVstMaxParamStrLen);
			break;

		case kFilterType:
			vst_strncpy(label, "Filter Type", kVstMaxParamStrLen);
			break;
	}
}

void	MiniVstEffect::getParameterDisplay(VstInt32 index, char *label)
{
	std::stringstream ss;

	switch(index) {
		case kCutOff:
			ss << (getSampleRate() * get_cutoff());
			break;

		case kdBGain:
			ss << get_db_gain();
			break;

		case kQ:
			ss << get_Q();
			break;

		case kFilterType:
			ss << defines::get_filter_string(get_filter_type());
			break;
	}

	vst_strncpy(label, ss.str().c_str(), kVstMaxParamStrLen);
}

void	MiniVstEffect::getParameterLabel(VstInt32 index, char *label)
{
	switch(index) {
		case kCutOff:
			vst_strncpy(label, "Hz", kVstMaxParamStrLen);
			break;

		case kdBGain:
			vst_strncpy(label, "dB", kVstMaxParamStrLen);
			break;

		case kQ:
			vst_strncpy(label, "", kVstMaxParamStrLen);
			break;

		case kFilterType:
			vst_strncpy(label, "", kVstMaxParamStrLen);
	}
}

bool	MiniVstEffect::getEffectName(char *name)
{
	vst_strncpy(name, defines::kEffect, kVstMaxEffectNameLen);
	return true;
}

bool	MiniVstEffect::getProductString(char *name)
{
	vst_strncpy(name, defines::kProduct, kVstMaxProductStrLen);
	return true;
}

bool	MiniVstEffect::getVendorString(char *name)
{
	vst_strncpy(name, defines::kVendor, kVstMaxVendorStrLen);
	return true;
}

VstInt32
		MiniVstEffect::getVendorVersion()
{
	return defines::kVendorVersion;
}

//! reset_coeffsで設定した係数を元に、IIRフィルタをかける
double	MiniVstEffect::process	(size_t channel, double input) const
{
	double const ret =
		(b0_/a0_) * input + (b1_/a0_) * x_[channel][0] + (b2_/a0_) * x_[channel][1]
						 - (a1_/a0_) * y_[channel][0] - (a2_/a0_) * y_[channel][1];

		x_[channel][1] = x_[channel][0];
		x_[channel][0] = input;
		y_[channel][1] = y_[channel][0];
		y_[channel][0] = ret;
		
	return ret;
}

void	MiniVstEffect::processReplacing	(float ** input, float ** output, VstInt32 sampleFrames)
{
	for(size_t ch = 0; ch < kNumChannels; ++ch) {

		float const * const x = input[ch];
		float * const y = output[ch];

		for(size_t i = 0; i < sampleFrames; ++i) {
			y[i] = static_cast<float>(process(ch, x[i]));
		}
	}
}

void	MiniVstEffect::processDoubleReplacing(double ** input, double ** output, VstInt32 sampleFrames)
{
	for(size_t ch = 0; ch < kNumChannels; ++ch) {

		double const * const x = input[ch];
		double * const y = output[ch];

		for(size_t i = 0; i < sampleFrames; ++i) {
			y[i] = process(ch, x[i]);
		}
	}
}

VstProgram &
		MiniVstEffect::get_current_program()
{
	return cur_program_;
}

VstProgram const &
		MiniVstEffect::get_current_program() const
{
	return cur_program_;
}

void	MiniVstEffect::clear_buffer()
{
	for(size_t ch = 0; ch < kNumChannels; ++ch) {
		for(size_t i = 0; i < 2; ++i) {
			x_[ch][i] = y_[ch][i] = 0.0;
		}
	}
}

void	MiniVstEffect::reset_coeffs	()
{
	double const Q = get_Q();
	double const A = pow(10.0, get_db_gain() / 40);
	double const w0 = 2.0 * M_PI * get_cutoff();
	double const cos_w0 = cos(w0);
	double const sin_w0 = sin(w0);
	double const alpha = sin_w0 / (2.0 * Q);
	double const K = 2.0 * sqrt(A) * alpha;

	switch(get_filter_type()) {
		case defines::LPF:
			b0_ = (1.0 - cos_w0) / 2.0;
			b1_ = 1.0 - cos_w0;
			b2_ = (1.0 - cos_w0) / 2.0;
			a0_ = 1.0 + alpha;
			a1_ = -2.0 * cos_w0;
			a2_ = 1.0 - alpha;
			break;

		case defines::HPF:
			b0_ = (1.0 + cos_w0) / 2.0;
			b1_ = -(1.0 + cos_w0);
			b2_ = (1.0 + cos_w0) / 2.0;
			a0_ = 1.0 + alpha;
			a1_ = -2.0 * cos_w0;
			a2_ = 1.0 - alpha;
			break;

		case defines::BPF:
			//(constant skirt gain, peak gain = Q)
			b0_ = sin_w0 / 2.0;
			b1_ = 0;
			b2_ = -sin_w0 / 2.0;
			a0_ = 1 + alpha;
			a1_ = -2 * cos_w0;
			a2_ = 1 - alpha;
			break;

		case defines::notch:
			b0_ = 1;
			b1_ = -2 * cos_w0;
			b2_ = 1;
			a0_ = 1 + alpha;
			a1_ = -2 * cos_w0;
			a2_ = 1 - alpha;
			break;

		case defines::APF:
			b0_ = 1.0 - alpha;
			b1_ = -2.0 * cos_w0;
			b2_ = 1.0 + alpha;
			a0_ = 1.0 + alpha;
			a1_ = -2.0 * cos_w0;
			a2_ = 1.0 - alpha;
			break;

		case defines::PeakingEQ:
			b0_ = 1 + alpha * A;
			b1_ = -2 * cos_w0;
			b2_ = 1 - alpha * A;
			a0_ = 1 + alpha / A;
			a1_ = -2.0 * cos_w0;
			a2_ = 1 - alpha / A;
			break;

		case defines::LowShelf:
			b0_ = A * ( (A+1) - (A-1) * cos_w0 + K );
			b1_ = 2 * A * ( (A-1) - (A+1) * cos_w0 );
			b2_ = A * ( (A+1) - (A-1) * cos_w0 - K );
			a0_ = (A+1) + (A-1) * cos_w0 + K;
			a1_ = -2 * ( (A-1) + (A+1) * cos_w0 );
			a2_ = (A+1) + (A-1) * cos_w0 - K;
			break;

		case defines::HighShelf:
			b0_ = A * ( (A+1) + (A-1) * cos_w0 + K );
			b1_ = -2 * A * ( (A-1) + (A+1) * cos_w0 );
			b2_ = A * ( (A+1) + (A-1) * cos_w0 - K );
			a0_ = (A+1) - (A-1) * cos_w0 + K;
			a1_ = 2 * ( (A-1) - (A+1) * cos_w0 );
			a2_ = (A+1) - (A-1) * cos_w0 - K;
			break;
	}
}

double	MiniVstEffect::get_db_gain		() const
{
	return
		defines::param_to_db(get_current_program().db_gain_);
}

//! @return normalized cutoff frequency
//! 0.x(30Hz) <= value <= 0.5
double	MiniVstEffect::get_cutoff		() const
{
	double const E = 10.0;
	double const e_range = E - 1.0;

	double const f_E = pow(E, (double)get_current_program().cutoff_);
	double const norm_f_E = (f_E - 1) / e_range;
	
	double const freq_range_reduce	= 75.0 / (get_sampling_rate() / 2.0);
	double const min_freq			= 30.0 / (get_sampling_rate() / 2.0);
	return 
		(norm_f_E / 2.0)	//0.0~0.5
		* (1-freq_range_reduce)		//0.0~0.4X(-75Hz)
		+ min_freq / 2.0;			//0.X(30Hz)~0.4X
}

double	MiniVstEffect::get_sampling_rate() const
{
	return const_cast<MiniVstEffect *>(this)->getSampleRate();
}

double	MiniVstEffect::get_Q			() const
{
	//! 0.3 - 18.0
	static double const q_range = 18.0 - 0.3;
	return 
		(get_current_program().Q_ * q_range) + 0.3;
}

size_t	MiniVstEffect::get_filter_type	() const
{
	return
		static_cast<size_t>(
			defines::param_to_filter(get_current_program().filter_type_)
			);
}

}	//namespace hwm

AudioEffect *
		createEffectInstance(audioMasterCallback audioMaster)
{
	return new hwm::MiniVstEffect(audioMaster);
}