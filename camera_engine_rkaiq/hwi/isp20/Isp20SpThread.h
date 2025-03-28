/*
 * Isp20SpThread.h - isp20 self-path frame reading thread for event and buffer
 *
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef _ISP20_SP_THREAD_H_
#define _ISP20_SP_THREAD_H_

#include "xcam_thread.h"
#include "xcam_mutex.h"
#include "motion_detect.h"
#include "rkisp2-config.h"

#define RK_AIQ_MOTION_DETECTION_VERSION_HEAD     "AIQ MDETECTION "
#define RK_AIQ_MOTION_DETECTION_VERSION_STR      "v2.0.2"
#define  RK_AIQ_MOTION_DETECTION_VERSION \
    RK_AIQ_MOTION_DETECTION_VERSION_HEAD \
    RK_AIQ_MOTION_DETECTION_VERSION_STR

#define MOTIONDETECT_SUBM                   (0x20)

using namespace XCam;

namespace RkCam {

#define CLIP(a, min_v, max_v)               (((a) < (min_v)) ? (min_v) : (((a) > (max_v)) ? (max_v) : (a)))
#define LOG_PRINT(a, b)         \
    if(1)                   \
        printf("%s",b);     \
    if(0)                       \
        fprintf(a, "%s", b);

#define ALIGN_UP(a, size)  ((a + size - 1) & (~(size - 1)))

typedef struct RKAnr_Mt_Params_t
{
    float   sigmaHScale[3][MAX_ISO_STEP];
    float   sigmaLScale[3][MAX_ISO_STEP];
    float   light_clp[3][MAX_ISO_STEP];;
    float   uv_weight[3][MAX_ISO_STEP];;
    float   mfnr_sigma_scale[3][MAX_ISO_STEP];
    float   yuvnr_gain_scale[3][MAX_ISO_STEP][3];
    float   reserved[10][3][MAX_ISO_STEP];

} RKAnr_Mt_Params_t;


enum MSG_CMD {
    MSG_CMD_WR_START,
    MSG_CMD_WR_EXIT,
};

typedef struct _sp_message_s {
    int                                 cmd;
    bool                                sync;
    SmartPtr<Mutex>                     mutex;
    SmartPtr<XCam::Cond>                cond;
    uint32_t                            arg1;
    SmartPtr<rk_aiq_tnr_buf_info_t>     arg2;
    void*                               arg3;
} sp_msg_t;

class NrProcUint;

class Isp20SpThread
    : public Thread
{
    friend class KgProcThread;
    friend class WrProcThread;
    friend class WrProcThread2;
public:
    Isp20SpThread               ();
    virtual ~Isp20SpThread      ();
    void set_sp_dev             (SmartPtr<V4l2SubDevice> ispdev, SmartPtr<V4l2Device> ispspdev,
                                 SmartPtr<V4l2SubDevice> isppdev, SmartPtr<V4l2SubDevice> snsdev,
                                 SmartPtr<V4l2SubDevice> lensdev, SmartPtr<NrProcUint> nrdev);
    void set_sp_img_size        (int w, int h, int w_align, int h_align, int ds_w, int ds_h);
    void set_af_img_size        (int w, int h, int w_align, int h_align);
    void set_gain_isp           (void *buf, uint8_t* ratio);
    void set_gain_wr                            (void *buf, uint8_t* ratio, uint8_t *gain_isp_buf_cur,  RKAnr_Mt_Params_Select_t mtParamsSelect_cur, uint16_t h_st, uint16_t h_end);
    void set_gainkg                             (void *buf, uint8_t* ratio, uint8_t* ratio_next, uint8_t *gain_isp_buf_cur, RKAnr_Mt_Params_Select_t mtParamsSelect_cur, RKAnr_Mt_Params_Select_t mtParamsSelect_last);
    void start                  ();
    void stop                   ();
    void pause                  ();
    void resume                 ();
    void set_calibDb(const CamCalibDbContext_t* calib);
    void set_working_mode(int mode);
    void update_motion_detection_params(ANRMotionParam_t *motion);
    void update_af_meas_params(rk_aiq_isp_af_meas_t *sp_meas);
    int get_lowpass_fv(uint32_t sequence, SmartPtr<V4l2BufferProxy> buf_proxy);
    void get_calibDb_thumbs_ds_size(unsigned int *ds_width, unsigned int *ds_height);
    bool set_tnr_info                          (SmartPtr<rk_aiq_tnr_buf_info_s> tnr_info);
protected:
    void init                                  ();
    void deinit                                ();
    virtual bool loop                          ();
    XCamReturn kg_proc_loop                    ();
    bool       wr_proc_loop                    ();
    bool       wr_proc_loop2                   ();
    int subscrible_ispgain_event               (bool on);
    int wait_ispgain_event                     (unsigned int event_type, struct v4l2_event *event);
    void destroy_stop_fds_ispsp                ();
    XCamReturn create_stop_fds_ispsp           ();
    void notify_stop_fds_exit                  ();
    void notify_wr_thread_exit                 ();
    void notify_wr2_thread_exit                ();
    bool notify_yg_cmd                         (SmartPtr<sp_msg_t> msg);
    bool notify_yg2_cmd                        (SmartPtr<sp_msg_t> msg);
    bool init_fbcbuf_fd                        ();
    XCamReturn select_motion_params(RKAnr_Mt_Params_Select_t *stmtParamsSelected, uint32_t frameid);
private:
    SmartPtr<V4l2Device>  _isp_sp_dev;
    SmartPtr<V4l2SubDevice>  _ispp_dev;
    SmartPtr<V4l2SubDevice>  _isp_dev;
    SmartPtr<V4l2SubDevice> _sensor_dev;
    SmartPtr<V4l2SubDevice> _focus_dev;
    SmartPtr<NrProcUint>    _nr_dev;
    SmartPtr<Thread> mKgThread;
    SmartPtr<Thread> mWrThread;
    SmartPtr<Thread> mWrThread2;
    bool _first_frame;
    int _working_mode;
    bool _gray_mode;

    float dynamicSw;
    mutable std::atomic<bool> _motion_detect_control;
    bool _motion_detect_control_proc;

    std::list<SmartPtr<V4l2BufferProxy>> _isp_buf_list;
    XCam::Mutex _buf_list_mutex;
    XCam::Mutex _motion_param_mutex;
    XCam::Mutex _afmeas_param_mutex;
    XCam::Mutex _motion_detect_mutex;
    int ispsp_stop_fds[2];
    int ispp_stop_fds[2];
    int sync_pipe_fd[2];
    SafeList<sp_msg_t>  _notifyYgCmdQ;
    SafeList<sp_msg_t>  _notifyYgCmdQ2;
    SafeList<rk_aiq_tnr_buf_info_t>  _kgWrList;
    const CamCalibDbContext_t *_calibDb;
    int _isp_fd_array[ISP2X_FBCBUF_FD_NUM];
    uint32_t _isp_idx_array[ISP2X_FBCBUF_FD_NUM];
    std::atomic<int> _isp_buf_num;
    std::atomic<bool> _fd_init_flag{true};

    int max_list_num;
    int first_frame;
    int frame_id_pp_upt;
    int frame_id_isp_upt;
    int frame_num_pp;
    int frame_num_isp;

    int img_ds_size_x;
    int img_ds_size_y;
    int _gain_width;
    int _gain_height;
    int _img_width_align;
    int _img_height_align;
    int img_width;
    int img_height;
    int img_width_align;
    int img_height_align;
    int img_buf_size;
    int img_buf_size_uv;
    int img_blk_isp_stride;


    int gain_width;
    int gain_height;
    int gain_tile_ispp_w;
    int gain_tile_ispp_h;
    int gain_tile_ispp_x;
    int gain_tile_ispp_y;
    int gainkg_unit;
    int gain_tile_gainkg_size;
    int gain_tile_gainkg_w;
    int gain_tile_gainkg_h;
    int gain_tile_gainkg_stride;
    int gain_kg_tile_h_align;
    int gainkg_tile_num;

    int gain_blk_isp_w;
    int gain_blk_isp_h;
    int gain_blk_isp_stride;

    int gain_blk_ispp_w;
    int gain_blk_ispp_stride;
    int gain_blk_ispp_h;
    int gain_blk_ispp_mem_size;

    uint8_t **static_ratio;
    uint8_t *frame_detect_flg;
    uint8_t **pImgbuf;
    uint8_t **gain_isp_buf_bak;
    uint8_t *pPreAlpha;


    RKAnr_Mt_Params_t           mtParams;
    RKAnr_Mt_Params_Select_t    mtParamsSelect;
    RKAnr_Mt_Params_Select_t    **mtParamsSelect_list;
    ANRMotionParam_t  _motion_params;

    uint8_t static_ratio_idx_in;
    uint8_t static_ratio_idx_out;
    uint8_t static_ratio_num;

    uint32_t static_frame_num;
    uint8_t static_ratio_l;
    uint8_t static_ratio_l_bit;
    uint8_t static_ratio_r_bit;
    int16_t *pTmpBuf;

    uint8_t *pAfTmp;
    uint32_t sub_shp4_4[RKAIQ_RAWAF_ROI_SUBWINS_NUM];
    uint32_t sub_shp8_8[RKAIQ_RAWAF_ROI_SUBWINS_NUM];
    uint32_t high_light[RKAIQ_RAWAF_ROI_SUBWINS_NUM];
    uint32_t high_light2[RKAIQ_RAWAF_ROI_SUBWINS_NUM];
    rk_aiq_af_algo_meas_t _af_meas_params;
    rk_aiq_lens_descriptor _lens_des;
    int af_img_width;
    int af_img_height;
    int af_img_width_align;
    int af_img_height_align;
};

class KgProcThread
    : public Thread
{
public:
    KgProcThread (Isp20SpThread *poll)
        : Thread ("kgThread")
        , _poll (poll)
    {}

protected:
    virtual bool loop () {
        XCamReturn ret = _poll->kg_proc_loop ();

        if (ret == XCAM_RETURN_NO_ERROR || ret == XCAM_RETURN_ERROR_TIMEOUT)
            return true;
        return false;
    }

private:
    Isp20SpThread *_poll;
};

class WrProcThread
    : public Thread
{
public:
    WrProcThread (Isp20SpThread *poll)
        : Thread ("wrThread")
        , _poll (poll)
    {}

protected:
    virtual bool loop () {
        return _poll->wr_proc_loop ();
    }

private:
    Isp20SpThread *_poll;
};

class WrProcThread2
    : public Thread
{
public:
    WrProcThread2 (Isp20SpThread *poll)
        : Thread ("wrThread2")
        , _poll (poll)
    {}

protected:
    virtual bool loop () {
        return _poll->wr_proc_loop2 ();
    }

private:
    Isp20SpThread *_poll;
};

}
#endif
