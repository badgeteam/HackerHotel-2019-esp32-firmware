import badge, easydraw, woezel, easywifi

lvl = badge.nvs_get_u8('ota', 'fixlvl', 0)
if lvl<4:
    # softmod for stereo sound
    if not badge.nvs_get_u16("modaudio", "mixer_ctl_0"):
        badge.nvs_set_u16("modaudio", "mixer_ctl_0", (0 << 15) + (0 << 8) + (1 << 7) + (32 << 0))
        badge.nvs_set_u16("modaudio", "mixer_ctl_1", (0 << 15) + (32 << 8) + (0 << 7) + (32 << 0))
    # only do once
    badge.nvs_set_u8('ota', 'fixlvl', 4)
    # YOLO: try to install the apps, if it doesn't work, don't do it on reboot
    easywifi.enable()
    for bloatware in ['event_schedule','de_rode_hack','hacker_gallery','angry_nerds_podcast']:
        # This is ugly, but if 1 app is already up to date we just try to fill the device with the rest
        try:
            woezel.install(bloatware)
        except:
            pass
