#include "./MiniVstEffect.hpp"
#define _USE_MATH_DEFINES

#include <cmath>
#include <cassert>
#include <sstream>
#include <string>
#include <vector>
#include "./MiniVstEffecteditor.h"

AudioEffect *
		createEffectInstance(audioMasterCallback audioMaster)
{
	return new hwm::MiniVstEffect(audioMaster);
}

namespace hwm {

//! 0.0 ~ 1.0
typedef float	vst_param_t;

struct VstProgram
{
	vst_param_t		cutoff_;
	vst_param_t		db_gain_;
	vst_param_t		Q_;	
	vst_param_t		filter_type_;

	std::string		name_;
};

struct defines{
	//! parameter index
	enum {
		kNumPrograms	= 8,
		kNumChannels	= 2,
		kVendorVersion	= 1
	};

	static int const	kID;
	static char const *kVendor;
	static char const *kProduct;
	static char const *kEffect;

	static char const *kProgNames[kNumPrograms];

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
	
	static
	size_t	param_to_filter(vst_param_t value)
	{
		return
			static_cast<size_t>(value * kNumFilterType - 0.5);
	}
	
	static
	vst_param_t
			filter_to_param(size_t filter)
	{
		return
			static_cast<vst_param_t>((filter + 0.5) / kNumFilterType);
	}
};

int	const defines::kID			= 'MVFx';
char const *	defines::kVendor		= "hotwatermorning";
char const *	defines::kProduct		= "Mini Vst Eq";
char const *	defines::kEffect		= defines::kProduct;
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

struct MiniVstEffect::Impl
{
	Impl(MiniVstEffect *eff)
		:	eff_(eff)
		,	b0_(0.0)
		,	b1_(0.0)
		,	b2_(0.0)
		,	a0_(0.0)
		,	a1_(0.0)
		,	a2_(0.0)
	{
		initialize();
	}

	MiniVstEffect	*eff_;
	double	b0_;
	double	b1_;
	double	b2_;
	double	a0_;
	double	a1_;
	double	a2_;
	double	mutable x_past_[defines::kNumChannels][2];
	double	mutable y_past_[defines::kNumChannels][2];

	//! 0.0~1.0‚Ìfloat’l‚ð-100.0‚©‚ç+12.0‚ÌdB’l‚ÉŠ„‚è“–‚Ä‚é
	static
	double	param_to_db(vst_param_t value)
	{
		return 20.0 * log10(static_cast<double>(value * 4.0));
	}

	static
	vst_param_t
			db_to_param(double dB)
	{
		return
			static_cast<vst_param_t>(pow(10.0, (dB)/20.0) / 4.0);
	}

	VstProgram cur_program_;

	void	initialize()
	{
		SetCurrentProgram(0);
	}
	
	void	clear_history()
	{
		for(size_t ch = 0; ch < defines::kNumChannels; ++ch) {
			for(size_t i = 0; i < 2; ++i) {
				x_past_[ch][i] = y_past_[ch][i] = 0.0;
			}
		}
	}

	void	SetParameter(VstInt32 index, vst_param_t value, bool reset_coeffs)
	{
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

		if(reset_coeffs) {
			this->reset_coeffs();
		}
		
		if(filter_changed) {
			clear_history();
		}
	}

	vst_param_t	GetParameter		(VstInt32 index) const
	{
		switch(index) {
			case kCutOff:
				return GetCurrentProgram().cutoff_;

			case kdBGain:
				return GetCurrentProgram().db_gain_;

			case kQ:
				return GetCurrentProgram().Q_;

			case kFilterType:
				return GetCurrentProgram().filter_type_;
		}

		return 0.0;
	}

	void	GetParameterName	(VstInt32 index, char *text) const
	{
		switch(index) {
			case kCutOff:
				vst_strncpy(text, "Cutoff", kVstMaxParamStrLen);
				break;

			case kdBGain:
				vst_strncpy(text, "dB Gain", kVstMaxParamStrLen);
				break;

			case kQ:
				vst_strncpy(text, "Q", kVstMaxParamStrLen);
				break;

			case kFilterType:
				vst_strncpy(text, "Filter Type", kVstMaxParamStrLen);
				break;
		}
	}

	char const *
			get_filter_string(size_t filter_type) const
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

	void	GetParameterDisplay(VstInt32 index, char *text) const
	{
		std::stringstream ss;

		switch(index) {
			case kCutOff:
				ss << (eff_->getSampleRate() * GetCutOff());
				break;

			case kdBGain:
				ss << GetdBGain();
				break;

			case kQ:
				ss << GetQ();
				break;

			case kFilterType:
				ss << get_filter_string(GetFilterType());
				break;
		}

		vst_strncpy(text, ss.str().c_str(), kVstMaxParamStrLen);
	}

	void	GetParameterLabel(VstInt32 index, char *text)
	{
		switch(index) {
			case kCutOff:
				vst_strncpy(text, "Hz", kVstMaxParamStrLen);
				break;

			case kdBGain:
				vst_strncpy(text, "dB", kVstMaxParamStrLen);
				break;

			case kQ:
				vst_strncpy(text, "", kVstMaxParamStrLen);
				break;

			case kFilterType:
				vst_strncpy(text, "", kVstMaxParamStrLen);
				break;
		}
	}

	void	SetCurrentProgram(size_t n)
	{
		cur_program_ = defines::presets[n];
		reset_coeffs();
		clear_history();
	}

	VstProgram const &
			GetCurrentProgram() const
	{
		return cur_program_;
	}

	void	GetProgramName			(char *text) const
	{
		vst_strncpy(text, cur_program_.name_.c_str(), kVstMaxProgNameLen);
	}

	void	SetProgramName			(char const *text)
	{
		get_current_program().name_ = text;
	}

	void	GetProgramNameIndexed	(VstInt32 index, char *text)
	{
		assert(index < defines::kNumPrograms);
		vst_strncpy(text, defines::presets[index].name_.c_str(), kVstMaxProgNameLen);
	}

	double	GetdBGain		() const
	{
		return
			param_to_db(GetCurrentProgram().db_gain_);
	}

	//! @return normalized cutoff frequency
	//! 0.x(30Hz) <= value <= 0.5
	double	GetCutOff		() const
	{
		double const E = 10.0;
		double const e_range = E - 1.0;

		double const f_E = pow(E, (double)GetCurrentProgram().cutoff_);
		double const norm_f_E = (f_E - 1) / e_range;
		
		double const freq_range_reduce	= 75.0 / (eff_->getSampleRate() / 2.0);
		double const min_freq			= 30.0 / (eff_->getSampleRate() / 2.0);
		return 
			(norm_f_E / 2.0)	//0.0~0.5
			* (1-freq_range_reduce)		//0.0~0.4X(-75Hz)
			+ min_freq / 2.0;			//0.X(30Hz)~0.4X
	}

	double	GetQ			() const
	{
		//! 0.3 - 18.0
		static double const q_range = 18.0 - 0.3;
		return 
			(GetCurrentProgram().Q_ * q_range) + 0.3;
	}

	size_t	GetFilterType	() const
	{
		return
			static_cast<size_t>(
				defines::param_to_filter(GetCurrentProgram().filter_type_)
				);
	}

	void	reset_coeffs	()
	{
		double const Q = GetQ();
		double const A = pow(10.0, GetdBGain() / 40);
		double const w0 = 2.0 * M_PI * GetCutOff();
		double const cos_w0 = cos(w0);
		double const sin_w0 = sin(w0);
		double const alpha = sin_w0 / (2.0 * Q);
		double const K = 2.0 * sqrt(A) * alpha;

		switch(GetFilterType()) {
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
	
	double	Process	(double input, size_t channel) const
	{
		double const ret =
			(b0_/a0_) * input + (b1_/a0_) * x_past_[channel][0] + (b2_/a0_) * x_past_[channel][1]
						     - (a1_/a0_) * y_past_[channel][0] - (a2_/a0_) * y_past_[channel][1];

			x_past_[channel][1] = x_past_[channel][0];
			x_past_[channel][0] = input;
			y_past_[channel][1] = y_past_[channel][0];
			y_past_[channel][0] = ret;
			
		return ret;
	}
	
	void	Process	(float const * const * input, float ** const output, size_t const sampleFrames) const
	{
		for(size_t ch = 0; ch < defines::kNumChannels; ++ch) {

			float const * const x = input[ch];
			float * const y = output[ch];

			for(size_t i = 0; i < sampleFrames; ++i) {
				y[i] = static_cast<float>(Process(x[i], ch));
			}
		}
	}

	void	Process	(double const * const * input, double ** const output, size_t const sampleFrames) const
	{
		for(size_t ch = 0; ch < defines::kNumChannels; ++ch) {

			double const * const x = input[ch];
			double * const y = output[ch];

			for(size_t i = 0; i < sampleFrames; ++i) {
				y[i] = Process(x[i], ch);
			}
		}
	}

private:
	VstProgram &
			get_current_program	()
	{
		return cur_program_;
	}
};

MiniVstEffect::MiniVstEffect(audioMasterCallback audioMaster)
	:	AudioEffectX(
			audioMaster,
			defines::kNumPrograms,
			kNumParams )
{
	pimpl_.reset(new Impl(this));
	//setEditor(VDAWVSTiEditor::GetEditorFactoryFunc()(this));
	
	setNumInputs(defines::kNumChannels);
	setNumOutputs(defines::kNumChannels);

	canProcessReplacing();
	//programsAreChunks();
	isSynth(false);
	setUniqueID(defines::kID);
	
	editor = new MiniVstEffectEditor(this);
}

MiniVstEffect::~MiniVstEffect()
{}

void	MiniVstEffect::setProgram		(VstInt32 program)
{
	pimpl_->SetCurrentProgram(program);
}

void	MiniVstEffect::setProgramName	(char *name)
{
	pimpl_->GetProgramName(name);
}

void	MiniVstEffect::getProgramName	(char *name)
{
	pimpl_->SetProgramName(name);
}

bool	MiniVstEffect::getProgramNameIndexed (VstInt32 /*unused*/, VstInt32 index, char* text)
{
	pimpl_->GetProgramNameIndexed(index, text);
	return true;
}

void	MiniVstEffect::setParameter		(VstInt32 index, vst_param_t value)
{
	AudioEffectX::setParameter(index, value);
	pimpl_->SetParameter(index, value, true);
	if (editor) {
		((AEffGUIEditor*)editor)->setParameter (index, value);
	}
}

vst_param_t
		MiniVstEffect::getParameter		(VstInt32 index)
{
	return pimpl_->GetParameter(index);
}

void	MiniVstEffect::getParameterName(VstInt32 index, char *label)
{
	pimpl_->GetParameterName(index, label);
}

void	MiniVstEffect::getParameterDisplay(VstInt32 index, char *label)
{
	pimpl_->GetParameterDisplay(index, label);
}

void	MiniVstEffect::getParameterLabel(VstInt32 index, char *label)
{
	pimpl_->GetParameterLabel(index, label);
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

void	MiniVstEffect::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	pimpl_->Process(inputs, outputs, sampleFrames);
}

void	MiniVstEffect::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	pimpl_->Process(inputs, outputs, sampleFrames);
}

}	//namespace hwm