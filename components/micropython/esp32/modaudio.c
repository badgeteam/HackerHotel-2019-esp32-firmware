#include <string.h>

#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "lib/utils/pyexec.h"

#include "extmod/vfs_native.h"

#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "wav_decoder.h"
#include "mp3_decoder.h"
#include "badge_pins.h"
#include "badge_nvs.h"
#include "badge_power.h"

#include "modaudio.h"

#ifdef IIS_SCLK

#define TAG "esp32/modaudio"

#define PERSISTENT_I2S_STREAM

void
modaudio_init(void)
{
    // initialize volume preference
    uint16_t value = 0;
    esp_err_t res = badge_nvs_get_u16("modaudio", "volume", &value);
    if ( res == ESP_OK ) {
        i2s_stream_volume = (value <= 1024) ? value : 1024;
    }

    // initialize mixer preferences
    value = 0;
    res = badge_nvs_get_u16("modaudio", "mixer_ctl_0", &value);
    if ( res == ESP_OK ) {
        int16_t v_0_0 = value & 0x7f;
        if (v_0_0 > 32) v_0_0 = 32;
        if (value & 0x80) v_0_0 = -v_0_0;
        int16_t v_0_1 = (value >> 8) & 0x7f;
        if (v_0_1 > 32) v_0_1 = 32;
        if (value & 0x8000) v_0_1 = -v_0_1;
        i2s_mixer_ctl_0_0 = v_0_0;
        i2s_mixer_ctl_0_1 = v_0_1;
    }

    value = 0;
    res = badge_nvs_get_u16("modaudio", "mixer_ctl_1", &value);
    if ( res == ESP_OK ) {
        int16_t v_1_0 = value & 0x7f;
        if (v_1_0 > 32) v_1_0 = 32;
        if (value & 0x80) v_1_0 = -v_1_0;
        int16_t v_1_1 = (value >> 8) & 0x7f;
        if (v_1_1 > 32) v_1_1 = 32;
        if (value & 0x8000) v_1_1 = -v_1_1;
        i2s_mixer_ctl_1_0 = v_1_0;
        i2s_mixer_ctl_1_1 = v_1_1;
    }
}

STATIC mp_obj_t audio_volume(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args > 0){
        int v = mp_obj_get_int(args[0]);
        if (v < 0) v = 0;
        else if (v > 1024) v = 1024;
        i2s_stream_volume = v;
    }

    return mp_obj_new_int(i2s_stream_volume);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(audio_volume_obj, 0, 1, audio_volume);

STATIC mp_obj_t audio_mixer_ctl_0(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        int v = mp_obj_get_int(args[0]);
        if (v < -32) v = -32;
        if (v >  32) v =  32;
        i2s_mixer_ctl_0_0 = v;
        i2s_mixer_ctl_0_1 = v;
    } else if (n_args == 2) {
        int v1 = mp_obj_get_int(args[0]);
        int v2 = mp_obj_get_int(args[1]);
        if (v1 < -32) v1 = -32;
        if (v1 >  32) v1 =  32;
        if (v2 < -32) v2 = -32;
        if (v2 >  32) v2 =  32;
        i2s_mixer_ctl_0_0 = v1;
        i2s_mixer_ctl_0_1 = v2;
    }

    mp_obj_t list_items[2];
    list_items[0] = mp_obj_new_int(i2s_mixer_ctl_0_0);
    list_items[1] = mp_obj_new_int(i2s_mixer_ctl_0_1);
    return mp_obj_new_list(2, list_items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(audio_mixer_ctl_0_obj, 0, 2, audio_mixer_ctl_0);

STATIC mp_obj_t audio_mixer_ctl_1(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        int v = mp_obj_get_int(args[0]);
        if (v < -32) v = -32;
        if (v >  32) v =  32;
        i2s_mixer_ctl_1_0 = v;
        i2s_mixer_ctl_1_1 = v;
    } else if (n_args == 2) {
        int v1 = mp_obj_get_int(args[0]);
        int v2 = mp_obj_get_int(args[1]);
        if (v1 < -32) v1 = -32;
        if (v1 >  32) v1 =  32;
        if (v2 < -32) v2 = -32;
        if (v2 >  32) v2 =  32;
        i2s_mixer_ctl_1_0 = v1;
        i2s_mixer_ctl_1_1 = v2;
    }

    mp_obj_t list_items[2];
    list_items[0] = mp_obj_new_int(i2s_mixer_ctl_1_0);
    list_items[1] = mp_obj_new_int(i2s_mixer_ctl_1_1);
    return mp_obj_new_list(2, list_items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(audio_mixer_ctl_1_obj, 0, 2, audio_mixer_ctl_1);

/* current active stream */
enum input_stream_type_t {
    INPUT_STREAM_FILE=1,
    INPUT_STREAM_HTTP,
};

const char *input_stream_str[] = {
    [INPUT_STREAM_FILE] "file",
    [INPUT_STREAM_HTTP] "http",
};

enum decoder_type_t {
    DECODER_WAV=1,
    DECODER_MP3,
};

const char *decoder_str[] = {
    [DECODER_WAV] "wav",
    [DECODER_MP3] "mp3",
};

static bool audio_stream_active = false;
static audio_event_iface_handle_t evt;
static audio_pipeline_handle_t pipeline = NULL;
// input stream
static enum input_stream_type_t input_stream_type = 0;
static audio_element_handle_t input_stream_reader = NULL;
// decoder
static enum decoder_type_t decoder_type = 0;
static audio_element_handle_t decoder = NULL;
// output stream
static audio_element_handle_t i2s_stream_writer = NULL;

static void
_modaudio_stream_cleanup(void)
{
    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);

    /* Terminate the pipeline before removing the listener */
    if (input_stream_reader != NULL) {
        audio_pipeline_unregister(pipeline, input_stream_reader);
    }

    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    audio_pipeline_unregister(pipeline, decoder);

    audio_pipeline_remove_listener(pipeline);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    evt = NULL;

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    pipeline = NULL;

    if (input_stream_reader != NULL) {
        audio_element_deinit(input_stream_reader);
        input_stream_reader = NULL;
        input_stream_type = 0;
    }

#ifndef PERSISTENT_I2S_STREAM
    audio_element_deinit(i2s_stream_writer);
    i2s_stream_writer = NULL;
#endif // ! PERSISTENT_I2S_STREAM

    if (decoder != NULL) {
        audio_element_deinit(decoder);
        decoder = NULL;
        decoder_type = 0;
    }

    audio_stream_active = false;
}

static void
_modaudio_event_listener_task(void *arg)
{
    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && decoder_type == DECODER_MP3
            && msg.source == (void *) decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {

            audio_element_info_t music_info = {0};
            audio_element_getinfo(decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    _modaudio_stream_cleanup();

    vTaskDelete(NULL);
}

static void _init_event(void) {
    static const audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    assert( evt != NULL );
}

static void _init_pipeline(void) {
    static const audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    assert( pipeline != NULL );
}

static void _init_fatfs_stream(void) {
    static const fatfs_stream_cfg_t fatfs_cfg = {
        .type        = AUDIO_STREAM_READER,
        .task_prio   = FATFS_STREAM_TASK_PRIO,
        .task_core   = FATFS_STREAM_TASK_CORE,
        .task_stack  = FATFS_STREAM_TASK_STACK,
        .out_rb_size = FATFS_STREAM_RINGBUFFER_SIZE,
        .buf_sz      = FATFS_STREAM_BUF_SIZE,
    };
    input_stream_type = INPUT_STREAM_FILE;
    input_stream_reader = fatfs_stream_init(&fatfs_cfg);
    assert( input_stream_reader != NULL );
}

static void _init_http_stream(void) {
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    input_stream_type = INPUT_STREAM_HTTP;
    input_stream_reader = http_stream_init(&http_cfg);
    assert( input_stream_reader != NULL );
}

static void _init_wav_decoder(void) {
    static const wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
    decoder_type = DECODER_WAV;
    decoder = wav_decoder_init(&wav_cfg);
    assert( decoder != NULL );
}

static void _init_mp3_decoder(void) {
    static const mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    decoder_type = DECODER_MP3;
    decoder = mp3_decoder_init(&mp3_cfg);
    assert( decoder != NULL );
}

static void _init_i2s_stream(void) {

#ifdef AUDIO_NEEDS_EXT_POWER
    badge_power_sdcard_enable();
#endif // AUDIO_NEEDS_EXT_POWER

#ifdef PERSISTENT_I2S_STREAM
    if (i2s_stream_writer != NULL) return;
#endif // PERSISTENT_I2S_STREAM

    static const i2s_stream_cfg_t i2s_cfg = {
        .type           = AUDIO_STREAM_WRITER,
        .task_prio      = I2S_STREAM_TASK_PRIO,
        .task_core      = I2S_STREAM_TASK_CORE,
        .task_stack     = I2S_STREAM_TASK_STACK,
        .out_rb_size    = I2S_STREAM_RINGBUFFER_SIZE,
        .i2s_config     = {
            .mode                 = I2S_MODE_MASTER | I2S_MODE_TX, // write only
            .sample_rate          = 44100, // set to 48000 ?
            .bits_per_sample      = 16,
            .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_I2S,
            .dma_buf_count        = 3,
            .dma_buf_len          = 300,
            .use_apll             = 0, // real sample-rate is too far off when enabled
            .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL2,
        },
        .i2s_pin_config = {
            .bck_io_num   = IIS_SCLK,
            .ws_io_num    = IIS_LCLK,
            .data_out_num = IIS_DSIN,
            .data_in_num  = IIS_DOUT,
        },
        .i2s_port       = 0,
    };

    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    assert( i2s_stream_writer != NULL );
}

STATIC mp_obj_t audio_is_playing(void) {
    return audio_stream_active ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(audio_is_playing_obj, audio_is_playing);

static mp_obj_t _audio_play_generic(const char *uri) {
    // configure pipeline
    audio_pipeline_register(pipeline, input_stream_reader, input_stream_str[input_stream_type]);
    audio_pipeline_register(pipeline, decoder, decoder_str[decoder_type]);
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    audio_pipeline_link(pipeline, (const char *[]) {input_stream_str[input_stream_type], decoder_str[decoder_type], "i2s"}, 3);

    // start stream
    audio_element_set_uri(input_stream_reader, uri);

    _init_event();

    audio_pipeline_set_listener(pipeline, evt);

    audio_pipeline_run(pipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {

            audio_element_info_t music_info = {0};
            audio_element_getinfo(decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from %s decoder, sample_rates=%d, bits=%d, ch=%d",
                     decoder_str[decoder_type], music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);

            // stream is playing; return
            xTaskCreate(&_modaudio_event_listener_task, "modaudio event-listener task", 4096, NULL, 10, NULL);
            return mp_const_true;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (int) msg.data == AEL_STATUS_STATE_STOPPED) {

            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    _modaudio_stream_cleanup();

    return mp_const_false;
}

STATIC mp_obj_t audio_play_wav_file(mp_obj_t _file) {
    const char *file_mp = mp_obj_str_get_str(_file);

    if (*file_mp == 0) { // empty string; keep as hack to test audio
        file_mp = "/sdcard/audio/ff-16b-2c-44100hz.wav";
    }

    char file[128] = {'\0'};
    int res = physicalPath(file_mp, file);
    if ((res != 0) || (strlen(file) == 0)) {
        mp_raise_ValueError("Error resolving file name");
        return mp_const_false;
    }

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_false;
    }

    audio_stream_active = true;
    _init_pipeline();
    _init_fatfs_stream();
    _init_i2s_stream();
    _init_wav_decoder();

    return _audio_play_generic(file);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_wav_file_obj, audio_play_wav_file);

STATIC mp_obj_t audio_play_wav_stream(mp_obj_t _url) {
    const char *url = mp_obj_str_get_str(_url);

    if (*url == 0) { // empty string; keep as hack to test audio
        url = "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.wav";
    }

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_false;
    }

    audio_stream_active = true;
    _init_pipeline();
    _init_http_stream();
    _init_i2s_stream();
    _init_wav_decoder();

    return _audio_play_generic(url);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_wav_stream_obj, audio_play_wav_stream);

STATIC mp_obj_t audio_play_mp3_file(mp_obj_t _file) {
    const char *file_mp = mp_obj_str_get_str(_file);

    if (*file_mp == 0) { // empty string; keep as hack to test audio
        file_mp = "/sdcard/audio/ff-16b-2c-44100hz.mp3";
    }

    char file[128] = {'\0'};
    int res = physicalPath(file_mp, file);
    if ((res != 0) || (strlen(file) == 0)) {
        mp_raise_ValueError("Error resolving file name");
        return mp_const_false;
    }

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_false;
    }

    audio_stream_active = true;
    _init_pipeline();
    _init_fatfs_stream();
    _init_i2s_stream();
    _init_mp3_decoder();

    return _audio_play_generic(file);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_mp3_file_obj, audio_play_mp3_file);

STATIC mp_obj_t audio_play_mp3_stream(mp_obj_t _url) {
    const char *url = mp_obj_str_get_str(_url);

    if (*url == 0) { // empty string; keep as hack to test audio
        url = "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3";
    }

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_false;
    }

    audio_stream_active = true;
    _init_pipeline();
    _init_http_stream();
    _init_i2s_stream();
    _init_mp3_decoder();

    return _audio_play_generic(url);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_mp3_stream_obj, audio_play_mp3_stream);

STATIC mp_obj_t audio_stop(void) {
    if (pipeline != NULL) {
        audio_pipeline_stop(pipeline);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(audio_stop_obj, audio_stop);

#else
void
modaudio_init(void)
{
    // dummy method; nothing to initialize
}
#endif // IIS_SCLK


// Module globals
STATIC const mp_rom_map_elem_t audio_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio)},

#ifdef IIS_SCLK
    {MP_OBJ_NEW_QSTR(MP_QSTR_volume), (mp_obj_t)&audio_volume_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_mixer_ctl_0), (mp_obj_t)&audio_mixer_ctl_0_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_mixer_ctl_1), (mp_obj_t)&audio_mixer_ctl_1_obj},

    {MP_OBJ_NEW_QSTR(MP_QSTR_is_playing), (mp_obj_t)&audio_is_playing_obj},

    {MP_OBJ_NEW_QSTR(MP_QSTR_play_wav_file), (mp_obj_t)&audio_play_wav_file_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_play_wav_stream), (mp_obj_t)&audio_play_wav_stream_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_play_mp3_file), (mp_obj_t)&audio_play_mp3_file_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_play_mp3_stream), (mp_obj_t)&audio_play_mp3_stream_obj},

    {MP_OBJ_NEW_QSTR(MP_QSTR_stop), (mp_obj_t)&audio_stop_obj},
#endif // IIS_SCLK
};

STATIC MP_DEFINE_CONST_DICT(audio_module_globals, audio_module_globals_table);

const mp_obj_module_t audio_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&audio_module_globals,
};
