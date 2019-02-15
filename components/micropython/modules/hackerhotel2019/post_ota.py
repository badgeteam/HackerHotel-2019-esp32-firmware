import badge, easydraw

lvl = badge.nvs_get_u8('ota', 'fixlvl', 0)
if lvl<3:
    # softmod for stereo sound
    if not badge.nvs_get_u16("modaudio", "mixer_ctl_0"):
        badge.nvs_set_u16("modaudio", "mixer_ctl_0", (0 << 15) + (0 << 8) + (1 << 7) + (32 << 0))
        badge.nvs_set_u16("modaudio", "mixer_ctl_1", (0 << 15) + (32 << 8) + (0 << 7) + (32 << 0))
    # only do once
    badge.nvs_set_u8('ota', 'fixlvl', 3)
