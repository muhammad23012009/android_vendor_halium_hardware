/*
 * Copyright (C) 2016 The Android Open Source Project
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
 */

#define LOG_TAG "audio_hw_primary"
#define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <log/log.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <media/audiohal/DeviceHalInterface.h>
#include <media/audiohal/DevicesFactoryHalInterface.h>
#include <media/audiohal/StreamHalInterface.h>

using namespace android;

struct wrapper_audio_device {
    struct audio_hw_device hw_device;
    sp<DeviceHalInterface> deviceIface;
};

struct wrapper_stream_in {
    struct audio_stream_in stream;
    sp<StreamInHalInterface> streamIface;
};

struct wrapper_stream_out {
    struct audio_stream_out stream;
    sp<StreamOutHalInterface> streamIface;
};

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    ALOGV("out_get_sample_rate");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    uint32_t rate = 0;
    audio_config_base_t configBase = {};

    if (out->streamIface->getAudioProperties(&configBase) == OK) {
        rate = configBase.sample_rate;
    }

    return rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("out_set_sample_rate: %d", rate);
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    ALOGV("out_get_buffer_size");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    size_t buffer_size = 0;

    out->streamIface->getBufferSize(&buffer_size);
    return buffer_size;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    ALOGV("out_get_channels");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    audio_channel_mask_t mask = {};
    audio_config_base_t configBase = {};

    if (out->streamIface->getAudioProperties(&configBase) == OK) {
        mask = configBase.channel_mask;
    }
    return mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGV("out_get_format");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    audio_format_t format = {};
    audio_config_base_t configBase = {};

    if (out->streamIface->getAudioProperties(&configBase) == OK) {
        format = configBase.format;
    }
    return format;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("out_set_format: %d",format);
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    ALOGV("out_standby");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    return out->streamIface->standby();
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("out_dump");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    return out->streamIface->dump(fd);
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    ALOGV("out_set_parameters");
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;

    String8 kvPairs(kvpairs);
    return out->streamIface->setParameters(kvPairs);
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGV("out_get_parameters");

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    String8 paramKeys;
    String8 values;

    out->streamIface->getParameters(paramKeys, &values);
    return strdup(values.string());
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    ALOGV("out_get_latency");
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;

    uint32_t latency = 0;
    out->streamIface->getLatency(&latency);
    return latency;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
        float right)
{
    ALOGV("out_set_volume: Left:%f Right:%f", left, right);
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    return out->streamIface->setVolume(left, right);
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
        size_t bytes)
{
    ALOGV("out_write: buffer: %p, bytes: %zu", buffer, bytes);

    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    size_t written = 0;

    status_t ret = out->streamIface->write(buffer, bytes, &written);
    if (ret != OK) {
        return ret;
    }

    return written;
}

static int out_get_render_position(const struct audio_stream_out *stream,
        uint32_t *dsp_frames)
{
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    *dsp_frames = 0;

    status_t ret = out->streamIface->getRenderPosition(dsp_frames);
    ALOGV("out_get_render_position: dsp_frames: %d", *dsp_frames);
    return ret;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
                                   uint64_t *frames, struct timespec *timestamp)
{
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    *frames = 0;

    status_t ret = out->streamIface->getPresentationPosition(frames, timestamp);
    ALOGV("out_get_presentation_position: frames: %lu", (unsigned long)(*frames));
    return ret;
}


static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("out_add_audio_effect: %p", effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("out_remove_audio_effect: %p", effect);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
        int64_t *timestamp)
{
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    *timestamp = 0;

    status_t ret = out->streamIface->getNextWriteTimestamp(timestamp);
    ALOGV("out_get_next_write_timestamp: %ld", (long int)(*timestamp));
    return ret;
}

/** audio_stream_in implementation **/

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    uint32_t rate = 0;
    audio_config_base_t configBase = {};

    if (in->streamIface->getAudioProperties(&configBase) == OK) {
        rate = configBase.sample_rate;
    }
    ALOGV("in_get_sample_rate: %d", rate);
    return rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("in_set_sample_rate: %d", rate);
    return -ENOSYS;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    audio_channel_mask_t channels = {};
    audio_config_base_t configBase = {};

    if (in->streamIface->getAudioProperties(&configBase) == OK) {
        channels = configBase.channel_mask;
    }
    ALOGV("in_get_channels: %d", channels);
    return channels;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    audio_format_t format = {};
    audio_config_base_t configBase = {};

    if (in->streamIface->getAudioProperties(&configBase) == OK) {
        format = configBase.format;
    }
    ALOGV("in_get_format: %d", format);
    return format;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;

    size_t buffer_size = 0;
    in->streamIface->getBufferSize(&buffer_size);

    ALOGV("in_get_buffer_size: %zu", buffer_size);
    return buffer_size;
}

static int in_standby(struct audio_stream *stream)
{
    ALOGV("in_standby");

    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    return in->streamIface->standby();
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("in_dump");

    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    return in->streamIface->dump(fd);
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    ALOGV("in_set_parameters");
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;

    String8 kvPairs(kvpairs);
    return in->streamIface->setParameters(kvPairs);
}

static char * in_get_parameters(const struct audio_stream *stream,
        const char *keys)
{
    ALOGV("in_get_parameters");

    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    String8 paramKeys;
    String8 values;

    in->streamIface->getParameters(paramKeys, &values);
    return strdup(values.string());
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    ALOGV("in_set_gain: %f", gain);

    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    return in->streamIface->setGain(gain);
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    ALOGV("in_read: bytes %zu", bytes);

    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    size_t read = 0;

    status_t ret = in->streamIface->read(buffer, bytes, &read);
    if (ret != OK) {
        return ret;
    }

    return read;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    uint32_t framesLost = 0;

    in->streamIface->getInputFramesLost(&framesLost);
    ALOGV("in_get_input_frames_lost: %d", framesLost);
    return framesLost;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        audio_output_flags_t flags,
        struct audio_config *config,
        struct audio_stream_out **stream_out,
        const char *address __unused)
{
    ALOGV("adev_open_output_stream...");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    struct wrapper_stream_out *out;
    int ret = 0;

    out = new struct wrapper_stream_out;
    if (!out)
        return -ENOMEM;

    status_t result = adev->deviceIface->openOutputStream(handle, devices, flags,
                                                           config, address, &out->streamIface);
    if (result != OK) {
        ALOGE("openOutputStream() error %d", result);
        delete out;
        return -EINVAL;
    }

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    ALOGI("adev_open_output_stream selects channel_mask=%d rate=%d format=%d",
          config->channel_mask, config->sample_rate, config->format);

    *stream_out = &out->stream;

    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
        struct audio_stream_out *stream)
{
    ALOGV("adev_close_output_stream...");
    struct wrapper_stream_out *out = (struct wrapper_stream_out *)stream;
    delete out;
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    ALOGV("adev_set_parameters");
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;

    String8 kvPairs(kvpairs);
    return adev->deviceIface->setParameters(kvPairs);
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGV("adev_get_parameters");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    String8 paramKeys;
    String8 values;

    adev->deviceIface->getParameters(paramKeys, &values);
    return strdup(values.string());
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGV("adev_init_check");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->initCheck();
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("adev_set_voice_volume: %f", volume);

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->setVoiceVolume(volume);
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("adev_set_master_volume: %f", volume);

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->setMasterVolume(volume);
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;

    status_t ret = adev->deviceIface->getMasterVolume(volume);
    ALOGV("adev_get_master_volume: %f", *volume);
    return ret;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    ALOGV("adev_set_master_mute: %d", muted);
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->setMasterMute(muted);
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;

    status_t ret = adev->deviceIface->getMasterMute(muted);
    ALOGV("adev_get_master_mute: %d", *muted);
    return ret;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    ALOGV("adev_set_mode: %d", mode);

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->setMode(mode);
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ALOGV("adev_set_mic_mute: %d", state);

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->setMicMute(state);
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ALOGV("adev_get_mic_mute");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->getMicMute(state);
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
        const struct audio_config *config)
{
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    size_t buffer_size = 0;

    adev->deviceIface->getInputBufferSize(config, &buffer_size);

    ALOGV("adev_get_input_buffer_size: %zu", buffer_size);
    return buffer_size;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        struct audio_config *config,
        struct audio_stream_in **stream_in,
        audio_input_flags_t flags __unused,
        const char *address __unused,
        audio_source_t source __unused)
{

    ALOGV("adev_open_input_stream...");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    struct wrapper_stream_in *in;
    int ret = 0;

    in = new struct wrapper_stream_in;
    if (!in)
        return -ENOMEM;

    status_t result = adev->deviceIface->openInputStream(handle, devices, config,
                                                         flags, address, source,
                                                         {}/*outputDevice*/, ""/*outputDeviceAddress*/,
                                                         &in->streamIface);
    if (result != OK) {
        ALOGE("openInputStream() error %d", result);
        delete in;
        return result;
    }

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    config->format = in_get_format(&in->stream.common);
    config->channel_mask = in_get_channels(&in->stream.common);
    config->sample_rate = in_get_sample_rate(&in->stream.common);

    ALOGI("adev_open_input_stream selects channel_mask=%d rate=%d format=%d",
          config->channel_mask, config->sample_rate, config->format);

    if (ret) {
        delete in;
    } else {
        *stream_in = &in->stream;
    }

    return ret;
 }

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    ALOGV("adev_close_input_stream...");
    struct wrapper_stream_in *in = (struct wrapper_stream_in *)stream;
    delete in;
}

static int adev_create_audio_patch(struct audio_hw_device *dev,
                                   unsigned int num_sources,
                                   const struct audio_port_config *sources,
                                   unsigned int num_sinks,
                                   const struct audio_port_config *sinks,
                                   audio_patch_handle_t *handle)
{
    ALOGV("adev_create_audio_patch");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->createAudioPatch(num_sources, sources,
                                               num_sinks, sinks,
                                               handle);
}

static int adev_release_audio_patch(struct audio_hw_device *dev,
                                    audio_patch_handle_t handle)
{
    ALOGV("adev_release_audio_patch");

    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)dev;
    return adev->deviceIface->releaseAudioPatch(handle);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGV("adev_dump");
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)device;
    return adev->deviceIface->dump(fd, {}/* args */);
}

static int adev_close(hw_device_t *device)
{
    ALOGV("adev_close");
    struct wrapper_audio_device *adev = (struct wrapper_audio_device *)device;
    delete adev;
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct wrapper_audio_device *adev;

    ALOGV("adev_open: %s", name);

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = new wrapper_audio_device;
    if (!adev)
        return -ENOMEM;

    sp<DevicesFactoryHalInterface> devicesFactoryHal = DevicesFactoryHalInterface::create();
    int rc = devicesFactoryHal->openDevice(AUDIO_HARDWARE_MODULE_ID_PRIMARY, &adev->deviceIface);
    if (rc) {
        ALOGE("devicesFactoryHal->openDevice() error %d loading module %s", rc, name);
    }

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;
    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.get_master_volume = adev_get_master_volume;
    adev->hw_device.set_master_mute = adev_set_master_mute;
    adev->hw_device.get_master_mute = adev_get_master_mute;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.create_audio_patch = adev_create_audio_patch;
    adev->hw_device.release_audio_patch = adev_release_audio_patch;
    adev->hw_device.dump = adev_dump;

    *device = &adev->hw_device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM __attribute__((visibility("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Audio wrapper HAL for Halium/PulseAudio",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
