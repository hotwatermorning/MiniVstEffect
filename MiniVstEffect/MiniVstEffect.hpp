#ifndef	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
#define	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP

#include "./vst_header_include.hpp"
#include <string>

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

struct MiniVstEffect
	:	AudioEffectX
{
	enum {
		kCutOff,
		kdBGain,
		kQ,
		kFilterType,
		kNumParams
	};
	
	enum {
		kNumChannels = 2
	};
	
	//============================================================================//
	//	ctor
	//============================================================================//
public:
	MiniVstEffect	(audioMasterCallback audioMaster);
	~MiniVstEffect	();
	
private:
	//! コピー禁止
	MiniVstEffect		(MiniVstEffect const &);
	MiniVstEffect &
			operator=	(MiniVstEffect const &);

	//============================================================================//
	//	parameters
	//============================================================================//
public:
	virtual void		setParameter		(VstInt32 index, float value);
	virtual float		getParameter		(VstInt32 index);
	virtual void		getParameterLabel	(VstInt32 index, char* label);
	virtual void		getParameterDisplay	(VstInt32 index, char* text);
	virtual void		getParameterName	(VstInt32 index, char* text);

	//============================================================================//
	//	programs
	//============================================================================//
public:
	virtual void		setProgram			(VstInt32 program);
	virtual void		setProgramName		(char* name);
	virtual void		getProgramName		(char* name);
	virtual bool		getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

	//============================================================================//
	//	plugin info
	//============================================================================//
public:
	virtual	bool		getEffectName		(char *name);
	virtual	bool		getVendorString		(char *text);
	virtual	bool		getProductString	(char *text);
	virtual	VstInt32	getVendorVersion	();
	
	//============================================================================//
	//	process (buffer to host)
	//============================================================================//
public:
	virtual	void		processReplacing		(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual	void		processDoubleReplacing	(double **inputs, double **outputs, VstInt32 sampleFrames);

	//============================================================================//
	//	process
	//============================================================================//
private:
	double	process(size_t channel, double input) const;
	
	double	b0_;
	double	b1_;
	double	b2_;
	double	a0_;
	double	a1_;
	double	a2_;
	double	mutable x_past_[kNumChannels][2];
	double	mutable y_past_[kNumChannels][2];

private:
	void	clear_buffer		();
	VstProgram &
			get_current_program	();
	VstProgram const &
			get_current_program	() const;
	void	reset_coeffs		();
	
	double	get_db_gain			() const;
	double	get_cutoff			() const;
	double	get_Q				() const;
	size_t	get_filter_type		() const;
	
	double	get_sampling_rate	() const;
			
private:
	double	sampling_rate_;	
	VstProgram	cur_program_;
};

}	//namespace hwm

#endif	//HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
