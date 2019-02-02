import badge, gc, uos

badge.mount_root()
uos.mount(uos.VfsNative(None), '/')

try:
	uos.mkdir('/lib')
except:
	pass

gc.collect()
