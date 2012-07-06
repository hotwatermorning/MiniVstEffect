#ifndef	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
#define	HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP

#include "./vst_header_include.hpp"
#include <string>

namespace hwm {

//! 0.0 ~ 1.0
typedef float	vst_param_t;
	
//! プログラム
struct VstProgram
{
	vst_param_t		cutoff_;
	vst_param_t		db_gain_;
	vst_param_t		Q_;	
	vst_param_t		filter_type_;

	std::string		name_;
};

//! プラグイン本体
struct MiniVstEffect
	//! AudioEffectXクラスを継承する
	:	AudioEffectX
{
	//! パラメータIDの定義
	enum {
		kCutOff,
		kdBGain,
		kQ,
		kFilterType,
		kNumParams
	};
	
	//! 入出力数の定義
	//! シンセなどでは入力0／出力2などにしたりする
	enum {
		kNumChannels = 2
	};
	
	//============================================================================//
	//	ctor
	//============================================================================//
public:
	MiniVstEffect	(audioMasterCallback audioMaster);
	~MiniVstEffect	();

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
	
private:
	VstProgram &
			get_current_program	();
	VstProgram const &
			get_current_program	() const;
			
	VstProgram	cur_program_;

	//============================================================================//
	//	plugin info
	//============================================================================//
public:
	virtual	bool		getEffectName		(char *name);
	virtual	bool		getVendorString		(char *text);
	virtual	bool		getProductString	(char *text);
	virtual	VstInt32	getVendorVersion	();
	
	//============================================================================//
	//	process
	//============================================================================//
public:
	virtual	void		processReplacing		(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual	void		processDoubleReplacing	(double **inputs, double **outputs, VstInt32 sampleFrames);

private:
	double	process(size_t channel, double input) const;
	
	//! bi-quadフィルタの係数
	double	b0_;
	double	b1_;
	double	b2_;
	double	a0_;
	double	a1_;
	double	a2_;
	//! 遅延子
	double	mutable x_[kNumChannels][2];
	double	mutable y_[kNumChannels][2];

private:
	//! bi-quadフィルタの遅延子をクリア
	void	clear_buffer		();
	
	//! フィルタの係数を再計算
	void	reset_coeffs		();

	//! 現在のパラメータの状態から、dBGainを取得
	//! dBGainは、Peaking EQ, Low Shelving, High Shelving以外のフィルタでは
	//! 使用されない
	double	get_db_gain			() const;
	//! 現在のパラメータの状態から、CutOffを正規化周波数で取得
	double	get_cutoff			() const;
	//! 現在のパラメータの状態から、Qを取得
	double	get_Q				() const;
	//! 現在のパラメータの状態から、フィルタのタイプを取得
	size_t	get_filter_type		() const;
	//! AudioEffectXからサンプリング周波数を取得
	double	get_sampling_rate	() const;
			
};

}	//namespace hwm

#endif	//HWM_MINIVSTEFFECT_MINIVSTEFFECT_HPP
