import gc
import uos
from flashbdev import bdev

if bdev:
    try:
        uos.mount(bdev, "/")
    except:
        import inisetup
        inisetup.setup()
	
gc.collect()
gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())
