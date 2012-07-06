#ifndef	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
#define	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP

#include "./vst_header_include.hpp"
#include <memory>

AudioEffect *
		createEffectInstance(audioMasterCallback audioMaster);

namespace hwm {

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
	
	//============================================================================//
	//	ctor
	//============================================================================//
public:
	MiniVstEffect	(audioMasterCallback audioMaster);
	~MiniVstEffect	();

	//============================================================================//
	//	process (buffer to host)
	//============================================================================//
public:
	virtual	void		processReplacing		(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual	void		processDoubleReplacing	(double **inputs, double **outputs, VstInt32 sampleFrames);

	//virtual	VstInt32	processEvents (VstEvents* events);

	//============================================================================//
	//	settings
	//============================================================================//
public:
	//virtual	void		setSampleRate		(float sampleRate);
	//virtual	float		getSampleRate		();

	//virtual	void		setBlockSize		(VstInt32 blockSize);
	//virtual	VstInt32	getBlockSize		();

	//============================================================================//
	//	i/o
	//============================================================================//
public:
	//virtual	bool		getOutputProperties	(VstInt32 index, VstPinProperties* prop);

	//============================================================================//
	//	operations
	//============================================================================//
public:
	//virtual	VstInt32	canDo				(char *text);
	//virtual	void		open				();
	//virtual	void		close				();
	//virtual	void		supend				();
	//virtual	void		resume				();

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
	//	load/save
	//============================================================================//
public:
	//virtual	VstInt32	getChunk			(void **data, bool as_program = false);
	//virtual	VstInt32	setChunk			(void *data, VstInt32 byteSize, bool as_program);

private:
	struct	Impl;
	std::auto_ptr<Impl>	pimpl_;

	//! コピー不可
	MiniVstEffect		(MiniVstEffect const &);
	MiniVstEffect &
			operator=	(MiniVstEffect const &);
};

}	//namespace hwm

#endif	//HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
