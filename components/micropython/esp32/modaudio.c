#include <string.h>

#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "badge_pins.h"
#include "badge_power.h"

#include "modaudio.h"

#ifdef IIS_SCLK

#define TAG "esp32/modaudio"

STATIC mp_obj_t audio_volume(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args > 0){
        int v = mp_obj_get_int(args[0]);
        if (v < 0) v = 0;
        else if (v > 128) v = 128;
        i2s_stream_volume = v;
    }

    return mp_obj_new_int(i2s_stream_volume);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(audio_volume_obj, 0, 1, audio_volume);

/* current active stream */
static bool audio_stream_active = false;
static audio_event_iface_handle_t evt;
static audio_pipeline_handle_t pipeline = NULL;
static audio_element_handle_t fatfs_stream_reader = NULL;
static audio_element_handle_t http_stream_reader = NULL;
static audio_element_handle_t wav_decoder = NULL;
static audio_element_handle_t mp3_decoder = NULL;
static audio_element_handle_t i2s_stream_writer = NULL;

static void
_modaudio_stream_cleanup(void)
{
    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);

    /* Terminate the pipeline before removing the listener */
    if (fatfs_stream_reader != NULL) {
        audio_pipeline_unregister(pipeline, fatfs_stream_reader);
    } else if (http_stream_reader != NULL) {
        audio_pipeline_unregister(pipeline, http_stream_reader);
    }

    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    if (wav_decoder != NULL) {
        audio_pipeline_unregister(pipeline, wav_decoder);
    } else if (mp3_decoder != NULL) {
        audio_pipeline_unregister(pipeline, mp3_decoder);
    }

    audio_pipeline_remove_listener(pipeline);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    evt = NULL;

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    pipeline = NULL;

    if (fatfs_stream_reader != NULL) {
        audio_element_deinit(fatfs_stream_reader);
        fatfs_stream_reader = NULL;
    } else if (http_stream_reader != NULL) {
        audio_element_deinit(http_stream_reader);
        http_stream_reader = NULL;
    }

    audio_element_deinit(i2s_stream_writer);
    i2s_stream_writer = NULL;

    if (wav_decoder != NULL) {
        audio_element_deinit(wav_decoder);
        wav_decoder = NULL;
    } else if (mp3_decoder != NULL) {
        audio_element_deinit(mp3_decoder);
        mp3_decoder = NULL;
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
            && msg.source == (void *) mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

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
    static fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER; // FIXME: to make const structure
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    assert( fatfs_stream_reader != NULL );
}

static void _init_http_stream(void) {
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_stream_reader = http_stream_init(&http_cfg);
    assert( http_stream_reader != NULL );
}

static void _init_mp3_decoder(void) {
    static const mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);
    assert( mp3_decoder != NULL );
}

static void _init_i2s_stream(void) {
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

STATIC mp_obj_t audio_play_mp3_file(mp_obj_t _file) {
    const char *file = mp_obj_str_get_str(_file);

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_none;
    }

    audio_stream_active = true;

#ifdef AUDIO_NEEDS_EXT_POWER
    badge_power_sdcard_enable();
#endif // AUDIO_NEEDS_EXT_POWER

    _init_pipeline();
    _init_fatfs_stream();
    _init_i2s_stream();
    _init_mp3_decoder();

    // configure pipeline
    audio_pipeline_register(pipeline, fatfs_stream_reader,"file");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    audio_pipeline_link(pipeline, (const char *[]) {"file", "mp3", "i2s"}, 3);

    // start stream
    if (*file == 0) { // empty string; keep as hack to test audio
        audio_element_set_uri(fatfs_stream_reader, "/sdcard/audio/ff-16b-2c-44100hz.mp3");
    } else {
        audio_element_set_uri(fatfs_stream_reader, file);
    }

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
            && msg.source == (void *) mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);

            // stream is playing; return
            xTaskCreate(&_modaudio_event_listener_task, "modaudio event-listener task", 4096, NULL, 10, NULL);
            return mp_const_none;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    _modaudio_stream_cleanup();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_mp3_file_obj, audio_play_mp3_file);

STATIC mp_obj_t audio_play_mp3_stream(mp_obj_t _url) {
    const char *url = mp_obj_str_get_str(_url);

    if (audio_stream_active) {
        ESP_LOGE(TAG, "another audio stream is already playing");
        return mp_const_none;
    }

    audio_stream_active = true;

#ifdef AUDIO_NEEDS_EXT_POWER
    badge_power_sdcard_enable();
#endif // AUDIO_NEEDS_EXT_POWER

    _init_pipeline();
    _init_http_stream();
    _init_i2s_stream();
    _init_mp3_decoder();

    // configure pipeline
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    audio_pipeline_link(pipeline, (const char *[]) {"http", "mp3", "i2s"}, 3);

    // start stream
    if (*url == 0) { // empty string; keep as hack to test audio
        audio_element_set_uri(http_stream_reader, "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3");
    } else {
        audio_element_set_uri(http_stream_reader, url);
    }

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
            && msg.source == (void *) mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);

            // stream is playing; return
            xTaskCreate(&_modaudio_event_listener_task, "modaudio event-listener task", 4096, NULL, 10, NULL);
            return mp_const_none;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    _modaudio_stream_cleanup();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_play_mp3_stream_obj, audio_play_mp3_stream);

STATIC mp_obj_t audio_stop(void) {
    if (pipeline != NULL) {
        audio_pipeline_stop(pipeline);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(audio_stop_obj, audio_stop);

#endif // IIS_SCLK


// Module globals
STATIC const mp_rom_map_elem_t audio_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio)},

#ifdef IIS_SCLK
    {MP_OBJ_NEW_QSTR(MP_QSTR_volume), (mp_obj_t)&audio_volume_obj},

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
