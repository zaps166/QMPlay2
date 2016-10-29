/* Types from: cuviddec.h and nvcuvid.h */

enum cudaVideoCodec
{
	cudaVideoCodec_MPEG1 = 0,
	cudaVideoCodec_MPEG2,
	cudaVideoCodec_MPEG4,
	cudaVideoCodec_VC1,
	cudaVideoCodec_H264,
	cudaVideoCodec_JPEG,
	cudaVideoCodec_H264_SVC,
	cudaVideoCodec_H264_MVC,
	cudaVideoCodec_HEVC,
	cudaVideoCodec_VP8,
	cudaVideoCodec_VP9,
};

enum cudaVideoChromaFormat
{
	cudaVideoChromaFormat_Monochrome = 0,
	cudaVideoChromaFormat_420,
	cudaVideoChromaFormat_422,
	cudaVideoChromaFormat_444
};

enum cudaVideoDeinterlaceMode
{
	cudaVideoDeinterlaceMode_Weave = 0,
	cudaVideoDeinterlaceMode_Bob,
	cudaVideoDeinterlaceMode_Adaptive
};

enum cudaVideoSurfaceFormat
{
	cudaVideoSurfaceFormat_NV12=0
};

enum cudaVideoCreateFlags
{
	cudaVideoCreate_Default     = 0x00,
	cudaVideoCreate_PreferCUDA  = 0x01,
	cudaVideoCreate_PreferDXVA  = 0x02,
	cudaVideoCreate_PreferCUVID = 0x04
};

enum CUvideopacketflags
{
	CUVID_PKT_ENDOFSTREAM   = 0x01,
	CUVID_PKT_TIMESTAMP     = 0x02,
	CUVID_PKT_DISCONTINUITY = 0x04
};

typedef qint64 CUvideotimestamp;
typedef void *CUvideoparser;
typedef void *CUvideodecoder;
typedef void *CUvideoctxlock;

struct CUVIDEOFORMAT
{
	cudaVideoCodec codec;

	struct
	{
		unsigned int numerator;
		unsigned int denominator;
	} frame_rate;
	unsigned char progressive_sequence;
	unsigned char bit_depth_luma_minus8;
	unsigned char bit_depth_chroma_minus8;
	unsigned char reserved1;
	unsigned int coded_width;
	unsigned int coded_height;

	struct
	{
		int left;
		int top;
		int right;
		int bottom;
	} display_area;
	cudaVideoChromaFormat chroma_format;
	unsigned int bitrate;

	struct
	{
		int x;
		int y;
	} display_aspect_ratio;

	struct
	{
		unsigned char video_format          : 3;
		unsigned char video_full_range_flag : 1;
		unsigned char reserved_zero_bits    : 4;
		unsigned char color_primaries;
		unsigned char transfer_characteristics;
		unsigned char matrix_coefficients;
	} video_signal_description;
	unsigned int seqhdr_data_length;
};

struct CUVIDMPEG2PICPARAMS
{
	int ForwardRefIdx;
	int BackwardRefIdx;
	int picture_coding_type;
	int full_pel_forward_vector;
	int full_pel_backward_vector;
	int f_code[2][2];
	int intra_dc_precision;
	int frame_pred_frame_dct;
	int concealment_motion_vectors;
	int q_scale_type;
	int intra_vlc_format;
	int alternate_scan;
	int top_field_first;

	unsigned char QuantMatrixIntra[64];
	unsigned char QuantMatrixInter[64];
};

struct CUVIDH264DPBENTRY
{
	int PicIdx;
	int FrameIdx;
	int is_long_term;
	int not_existing;
	int used_for_reference;
	int FieldOrderCnt[2];
};

struct CUVIDH264MVCEXT
{
	int num_views_minus1;
	int view_id;
	unsigned char inter_view_flag;
	unsigned char num_inter_view_refs_l0;
	unsigned char num_inter_view_refs_l1;
	unsigned char MVCReserved8Bits;
	int InterViewRefsL0[16];
	int InterViewRefsL1[16];
};

struct CUVIDH264SVCEXT
{
	unsigned char profile_idc;
	unsigned char level_idc;
	unsigned char DQId;
	unsigned char DQIdMax;
	unsigned char disable_inter_layer_deblocking_filter_idc;
	unsigned char ref_layer_chroma_phase_y_plus1;
	signed char   inter_layer_slice_alpha_c0_offset_div2;
	signed char   inter_layer_slice_beta_offset_div2;

	unsigned short DPBEntryValidFlag;
	unsigned char inter_layer_deblocking_filter_control_present_flag;
	unsigned char extended_spatial_scalability_idc;
	unsigned char adaptive_tcoeff_level_prediction_flag;
	unsigned char slice_header_restriction_flag;
	unsigned char chroma_phase_x_plus1_flag;
	unsigned char chroma_phase_y_plus1;

	unsigned char tcoeff_level_prediction_flag;
	unsigned char constrained_intra_resampling_flag;
	unsigned char ref_layer_chroma_phase_x_plus1_flag;
	unsigned char store_ref_base_pic_flag;
	unsigned char Reserved8BitsA;
	unsigned char Reserved8BitsB;

	short scaled_ref_layer_left_offset;
	short scaled_ref_layer_top_offset;
	short scaled_ref_layer_right_offset;
	short scaled_ref_layer_bottom_offset;
	unsigned short Reserved16Bits;
	struct _CUVIDPICPARAMS *pNextLayer;
	int bRefBaseLayer;
};

struct CUVIDH264PICPARAMS
{
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int frame_mbs_only_flag;
	int direct_8x8_inference_flag;
	int num_ref_frames;
	unsigned char residual_colour_transform_flag;
	unsigned char bit_depth_luma_minus8;
	unsigned char bit_depth_chroma_minus8;
	unsigned char qpprime_y_zero_transform_bypass_flag;

	int entropy_coding_mode_flag;
	int pic_order_present_flag;
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;
	int weighted_pred_flag;
	int weighted_bipred_idc;
	int pic_init_qp_minus26;
	int deblocking_filter_control_present_flag;
	int redundant_pic_cnt_present_flag;
	int transform_8x8_mode_flag;
	int MbaffFrameFlag;
	int constrained_intra_pred_flag;
	int chroma_qp_index_offset;
	int second_chroma_qp_index_offset;
	int ref_pic_flag;
	int frame_num;
	int CurrFieldOrderCnt[2];

	CUVIDH264DPBENTRY dpb[16];

	unsigned char WeightScale4x4[6][16];
	unsigned char WeightScale8x8[2][64];

	unsigned char fmo_aso_enable;
	unsigned char num_slice_groups_minus1;
	unsigned char slice_group_map_type;
	signed char pic_init_qs_minus26;
	unsigned int slice_group_change_rate_minus1;
	union
	{
		unsigned long long slice_group_map_addr;
		const unsigned char *pMb2SliceGroupMap;
	} fmo;
	unsigned int  Reserved[12];

	union
	{
		CUVIDH264MVCEXT mvcext;
		CUVIDH264SVCEXT svcext;
	};
};

struct CUVIDVC1PICPARAMS
{
	int ForwardRefIdx;
	int BackwardRefIdx;
	int FrameWidth;
	int FrameHeight;

	int intra_pic_flag;
	int ref_pic_flag;
	int progressive_fcm;

	int profile;
	int postprocflag;
	int pulldown;
	int interlace;
	int tfcntrflag;
	int finterpflag;
	int psf;
	int multires;
	int syncmarker;
	int rangered;
	int maxbframes;

	int panscan_flag;
	int refdist_flag;
	int extended_mv;
	int dquant;
	int vstransform;
	int loopfilter;
	int fastuvmc;
	int overlap;
	int quantizer;
	int extended_dmv;
	int range_mapy_flag;
	int range_mapy;
	int range_mapuv_flag;
	int range_mapuv;
	int rangeredfrm;
};

struct CUVIDMPEG4PICPARAMS
{
	int ForwardRefIdx;
	int BackwardRefIdx;

	int video_object_layer_width;
	int video_object_layer_height;
	int vop_time_increment_bitcount;
	int top_field_first;
	int resync_marker_disable;
	int quant_type;
	int quarter_sample;
	int short_video_header;
	int divx_flags;

	int vop_coding_type;
	int vop_coded;
	int vop_rounding_type;
	int alternate_vertical_scan_flag;
	int interlaced;
	int vop_fcode_forward;
	int vop_fcode_backward;
	int trd[2];
	int trb[2];

	unsigned char QuantMatrixIntra[64];
	unsigned char QuantMatrixInter[64];
	int gmc_enabled;
};

struct CUVIDJPEGPICPARAMS
{
	int Reserved;
};

struct CUVIDHEVCPICPARAMS
{
	int pic_width_in_luma_samples;
	int pic_height_in_luma_samples;
	unsigned char log2_min_luma_coding_block_size_minus3;
	unsigned char log2_diff_max_min_luma_coding_block_size;
	unsigned char log2_min_transform_block_size_minus2;
	unsigned char log2_diff_max_min_transform_block_size;
	unsigned char pcm_enabled_flag;
	unsigned char log2_min_pcm_luma_coding_block_size_minus3;
	unsigned char log2_diff_max_min_pcm_luma_coding_block_size;
	unsigned char pcm_sample_bit_depth_luma_minus1;

	unsigned char pcm_sample_bit_depth_chroma_minus1;
	unsigned char pcm_loop_filter_disabled_flag;
	unsigned char strong_intra_smoothing_enabled_flag;
	unsigned char max_transform_hierarchy_depth_intra;
	unsigned char max_transform_hierarchy_depth_inter;
	unsigned char amp_enabled_flag;
	unsigned char separate_colour_plane_flag;
	unsigned char log2_max_pic_order_cnt_lsb_minus4;

	unsigned char num_short_term_ref_pic_sets;
	unsigned char long_term_ref_pics_present_flag;
	unsigned char num_long_term_ref_pics_sps;
	unsigned char sps_temporal_mvp_enabled_flag;
	unsigned char sample_adaptive_offset_enabled_flag;
	unsigned char scaling_list_enable_flag;
	unsigned char IrapPicFlag;
	unsigned char IdrPicFlag;

	unsigned char bit_depth_luma_minus8;
	unsigned char bit_depth_chroma_minus8;
	unsigned char reserved1[14];

	unsigned char dependent_slice_segments_enabled_flag;
	unsigned char slice_segment_header_extension_present_flag;
	unsigned char sign_data_hiding_enabled_flag;
	unsigned char cu_qp_delta_enabled_flag;
	unsigned char diff_cu_qp_delta_depth;
	signed char init_qp_minus26;
	signed char pps_cb_qp_offset;
	signed char pps_cr_qp_offset;

	unsigned char constrained_intra_pred_flag;
	unsigned char weighted_pred_flag;
	unsigned char weighted_bipred_flag;
	unsigned char transform_skip_enabled_flag;
	unsigned char transquant_bypass_enabled_flag;
	unsigned char entropy_coding_sync_enabled_flag;
	unsigned char log2_parallel_merge_level_minus2;
	unsigned char num_extra_slice_header_bits;

	unsigned char loop_filter_across_tiles_enabled_flag;
	unsigned char loop_filter_across_slices_enabled_flag;
	unsigned char output_flag_present_flag;
	unsigned char num_ref_idx_l0_default_active_minus1;
	unsigned char num_ref_idx_l1_default_active_minus1;
	unsigned char lists_modification_present_flag;
	unsigned char cabac_init_present_flag;
	unsigned char pps_slice_chroma_qp_offsets_present_flag;

	unsigned char deblocking_filter_override_enabled_flag;
	unsigned char pps_deblocking_filter_disabled_flag;
	signed char pps_beta_offset_div2;
	signed char pps_tc_offset_div2;
	unsigned char tiles_enabled_flag;
	unsigned char uniform_spacing_flag;
	unsigned char num_tile_columns_minus1;
	unsigned char num_tile_rows_minus1;

	unsigned short column_width_minus1[21];
	unsigned short row_height_minus1[21];
	unsigned int reserved3[15];

	int NumBitsForShortTermRPSInSlice;
	int NumDeltaPocsOfRefRpsIdx;
	int NumPocTotalCurr;
	int NumPocStCurrBefore;
	int NumPocStCurrAfter;
	int NumPocLtCurr;
	int CurrPicOrderCntVal;
	int RefPicIdx[16];
	int PicOrderCntVal[16];
	unsigned char IsLongTerm[16];
	unsigned char RefPicSetStCurrBefore[8];
	unsigned char RefPicSetStCurrAfter[8];
	unsigned char RefPicSetLtCurr[8];
	unsigned char RefPicSetInterLayer0[8];
	unsigned char RefPicSetInterLayer1[8];
	unsigned int reserved4[12];

	unsigned char ScalingList4x4[6][16];
	unsigned char ScalingList8x8[6][64];
	unsigned char ScalingList16x16[6][64];
	unsigned char ScalingList32x32[2][64];
	unsigned char ScalingListDCCoeff16x16[6];
	unsigned char ScalingListDCCoeff32x32[2];
};

struct CUVIDVP8PICPARAMS
{
	int width;
	int height;
	unsigned int first_partition_size;

	unsigned char LastRefIdx;
	unsigned char GoldenRefIdx;
	unsigned char AltRefIdx;
	union
	{
		struct
		{
			unsigned char frame_type : 1;
			unsigned char version : 3;
			unsigned char show_frame : 1;
			unsigned char update_mb_segmentation_data : 1;
			unsigned char Reserved2Bits : 2;
		};
		unsigned char wFrameTagFlags;
	};
	unsigned char Reserved1[4];
	unsigned int  Reserved2[3];
};

struct CUVIDVP9PICPARAMS
{
	unsigned int width;
	unsigned int height;

	unsigned char LastRefIdx;
	unsigned char GoldenRefIdx;
	unsigned char AltRefIdx;
	unsigned char colorSpace;

	unsigned short profile : 3;
	unsigned short frameContextIdx : 2;
	unsigned short frameType : 1;
	unsigned short showFrame : 1;
	unsigned short errorResilient : 1;
	unsigned short frameParallelDecoding : 1;
	unsigned short subSamplingX : 1;
	unsigned short subSamplingY : 1;
	unsigned short intraOnly : 1;
	unsigned short allow_high_precision_mv : 1;
	unsigned short refreshEntropyProbs : 1;
	unsigned short reserved2Bits : 2;

	unsigned short reserved16Bits;

	unsigned char  refFrameSignBias[4];

	unsigned char bitDepthMinus8Luma;
	unsigned char bitDepthMinus8Chroma;
	unsigned char loopFilterLevel;
	unsigned char loopFilterSharpness;

	unsigned char modeRefLfEnabled;
	unsigned char log2_tile_columns;
	unsigned char log2_tile_rows;

	unsigned char segmentEnabled : 1;
	unsigned char segmentMapUpdate : 1;
	unsigned char segmentMapTemporalUpdate : 1;
	unsigned char segmentFeatureMode : 1;
	unsigned char reserved4Bits : 4;

	unsigned char segmentFeatureEnable[8][4];
	short segmentFeatureData[8][4];
	unsigned char mb_segment_tree_probs[7];
	unsigned char segment_pred_probs[3];
	unsigned char reservedSegment16Bits[2];

	int qpYAc;
	int qpYDc;
	int qpChDc;
	int qpChAc;

	unsigned int activeRefIdx[3];
	unsigned int resetFrameContext;
	unsigned int mcomp_filter_type;
	unsigned int mbRefLfDelta[4];
	unsigned int mbModeLfDelta[2];
	unsigned int frameTagSize;
	unsigned int offsetToDctParts;
	unsigned int reserved128Bits[4];
};

struct CUVIDPICPARAMS
{
	int PicWidthInMbs;
	int FrameHeightInMbs;
	int CurrPicIdx;
	int field_pic_flag;
	int bottom_field_flag;
	int second_field;

	unsigned int nBitstreamDataLen;
	const unsigned char *pBitstreamData;
	unsigned int nNumSlices;
	const unsigned int *pSliceDataOffsets;
	int ref_pic_flag;
	int intra_pic_flag;
	unsigned int Reserved[30];

	union
	{
		CUVIDMPEG2PICPARAMS mpeg2;
		CUVIDH264PICPARAMS h264;
		CUVIDVC1PICPARAMS vc1;
		CUVIDMPEG4PICPARAMS mpeg4;
		CUVIDJPEGPICPARAMS jpeg;
		CUVIDHEVCPICPARAMS hevc;
		CUVIDVP8PICPARAMS vp8;
		CUVIDVP9PICPARAMS vp9;
		unsigned int CodecReserved[1024];
	} CodecSpecific;
};

struct CUVIDPARSERDISPINFO
{
	int picture_index;
	int progressive_frame;
	int top_field_first;
	int repeat_first_field;
	CUvideotimestamp timestamp;
};

struct CUVIDEOFORMATEX
{
	CUVIDEOFORMAT format;
	unsigned char raw_seqhdr_data[1024];
};

typedef int (CUDAAPI *PFNVIDSEQUENCECALLBACK)(void *, CUVIDEOFORMAT *);
typedef int (CUDAAPI *PFNVIDDECODECALLBACK)(void *, CUVIDPICPARAMS *);
typedef int (CUDAAPI *PFNVIDDISPLAYCALLBACK)(void *, CUVIDPARSERDISPINFO *);

struct CUVIDPARSERPARAMS
{
	cudaVideoCodec CodecType;
	unsigned int ulMaxNumDecodeSurfaces;
	unsigned int ulClockRate;
	unsigned int ulErrorThreshold;
	unsigned int ulMaxDisplayDelay;
	unsigned int uReserved1[5];
	void *pUserData;
	PFNVIDSEQUENCECALLBACK pfnSequenceCallback;
	PFNVIDDECODECALLBACK pfnDecodePicture;
	PFNVIDDISPLAYCALLBACK pfnDisplayPicture;
	void *pvReserved2[7];
	CUVIDEOFORMATEX *pExtVideoInfo;
};

struct CUVIDDECODECREATEINFO
{
	unsigned long ulWidth;
	unsigned long ulHeight;
	unsigned long ulNumDecodeSurfaces;
	cudaVideoCodec CodecType;
	cudaVideoChromaFormat ChromaFormat;
	unsigned long ulCreationFlags;
	unsigned long bitDepthMinus8;
	unsigned long Reserved1[4];

	struct {
		short left;
		short top;
		short right;
		short bottom;
	} display_area;

	cudaVideoSurfaceFormat OutputFormat;
	cudaVideoDeinterlaceMode DeinterlaceMode;
	unsigned long ulTargetWidth;
	unsigned long ulTargetHeight;
	unsigned long ulNumOutputSurfaces;
	CUvideoctxlock vidLock;

	struct {
		short left;
		short top;
		short right;
		short bottom;
	} target_rect;
	unsigned long Reserved2[5];
};

struct CUVIDPROCPARAMS
{
	int progressive_frame;
	int second_field;
	int top_field_first;
	int unpaired_field;

	unsigned int reserved_flags;
	unsigned int reserved_zero;
	unsigned long long raw_input_dptr;
	unsigned int raw_input_pitch;
	unsigned int raw_input_format;
	unsigned long long raw_output_dptr;
	unsigned int raw_output_pitch;
	unsigned int Reserved[48];
	void *Reserved3[3];
};

struct CUVIDSOURCEDATAPACKET
{
	unsigned long flags;
	unsigned long payload_size;
	const unsigned char *payload;
	CUvideotimestamp timestamp;
};
