import ugfx, badge, appglue, dialogs, easydraw, time

def asked_nickname(value):
    if value:
        badge.nvs_set_str("owner", "name", value)
    appglue.home()

ugfx.init()
nickname = badge.nvs_get_str("owner", "name", "")
dialogs.prompt_text("Nickname", nickname, cb=asked_nickname)
