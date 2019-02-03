import badge, easydraw

def u0to1():
    easydraw.msg("Applied patch 1")

def u1to2():
    badge.nvs_set_u8('sponsors', 'shown', 0)
    easydraw.msg("Applied patch 2")

# Read current patch level
lvl = badge.nvs_get_u8('ota', 'fixlvl', 0)

if lvl<1:
    easydraw.msg("Running post OTA scripts...","Still updating anyways...",)
    u0to1()
    easydraw.msg("Post OTA update completed")
    badge.nvs_set_u8('ota', 'fixlvl', 1)

if lvl<2:
    easydraw.msg("Running post OTA scripts...","Still updating anyways...",)
    u1to2()
    easydraw.msg("Post OTA update completed")
    badge.nvs_set_u8('ota', 'fixlvl', 2)
