import badge, gc, uos

badge.mount_root()
uos.mount(uos.VfsNative(None), '/')

gc.collect()
