import artemis

class FlashBdev:
    SEC_SIZE = 8192

    def __init__(self, start_sec, blocks):
        self.start_sec = start_sec
        self.blocks = blocks

    def readblocks(self, n, buf, off=0):
        # print("readblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        artemis.flash_read((n + self.start_sec) * self.SEC_SIZE + off, buf)

    def writeblocks(self, n, buf, off=None):
        # print("writeblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        if off is None:
            artemis.flash_erase(n + self.start_sec)
            off = 0
        artemis.flash_write((n + self.start_sec) * self.SEC_SIZE + off, buf)

    def ioctl(self, op, arg):
        # print("ioctl(%d, %r)" % (op, arg))
        if op == 4:  # MP_BLOCKDEV_IOCTL_BLOCK_COUNT
            return self.blocks
        if op == 5:  # MP_BLOCKDEV_IOCTL_BLOCK_SIZE
            return self.SEC_SIZE
        if op == 6:  # MP_BLOCKDEV_IOCTL_BLOCK_ERASE
            artemis.flash_erase(arg + self.start_sec)
            return 0


size = artemis.flash_size()
if size < 1024 * 1024:
    bdev = None
else:
    start_sec = (artemis.flash_user_start() // FlashBdev.SEC_SIZE) + 1
    # print("VFS Flash start = %x %x" % (artemis.flash_user_start(), start_sec))
    bdev = FlashBdev(start_sec, size // FlashBdev.SEC_SIZE - start_sec)
