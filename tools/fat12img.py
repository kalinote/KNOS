import os
import sys
import struct
from datetime import datetime

FD_SIZE = 1474560
FAT1_ADDRESS = 0x0200
FAT_BYTES = 0x1200
FAT2_ADDRESS = 0x1400
DIR_ADDRESS = 0x2600
ENTITY_ADDRESS = 0x3e00
FILEINFO_BYTES = 32
FILE_COUNT = 224
CLUSTER_BYTES = 512
FILE_NORMAL = 0x20

def setstrlen(s, length, pad=' '):
    return s.ljust(length, pad)[:length]

def filename83(filename):
    name, ext = os.path.splitext(os.path.basename(filename))
    fname = setstrlen(name, 8)
    ext = setstrlen(ext[1:], 3)
    return fname + ext

def calc_timestamp(time):
    d = ((time.year - 1980) << 9) | (time.month << 5) | time.day
    t = (time.hour << 11) | (time.minute << 5) | (time.second // 2)
    return d, t

class Fat:
    def __init__(self, filename):
        self.filename = filename
        with open(filename, 'rb') as f:
            f.seek(FAT1_ADDRESS)
            data = f.read(FAT_BYTES)
        self.fat = []
        for i in range(0, len(data), 3):
            n1 = data[i] | ((data[i+1] & 0x0F) << 8)
            n2 = (data[i+1] >> 4) | (data[i+2] << 4)
            self.fat.extend([n1, n2])

    def write(self):
        data = bytearray()
        for i in range(0, len(self.fat), 2):
            n1, n2 = self.fat[i], self.fat[i+1]
            data.extend([n1 & 0xFF, ((n2 & 0x0F) << 4) | (n1 >> 8), n2 >> 4])
        with open(self.filename, 'r+b') as f:
            f.seek(FAT1_ADDRESS)
            f.write(data)
            f.seek(FAT2_ADDRESS)
            f.write(data)

    def have_space(self, cluster_count):
        start = None
        n = 0
        for i, x in enumerate(self.fat):
            if x == 0:
                if start is None:
                    start = i
                n += 1
                if n >= cluster_count:
                    return start
        return False

    def find_free_cluster(self, start):
        for i in range(start, len(self.fat)):
            if self.fat[i] == 0:
                return i
        return False

    def put_next(self, cluster, value):
        self.fat[cluster] = value

    def get_next(self, cluster):
        return self.fat[cluster]

    def del_chain(self, cluster, size):
        no = (size + CLUSTER_BYTES - 1) // CLUSTER_BYTES
        for _ in range(no):
            next_cluster = self.fat[cluster]
            self.fat[cluster] = 0
            if next_cluster >= 0xFF8:
                break
            cluster = next_cluster

class FileInfo:
    def __init__(self, filename, type, time, date, cluster, size):
        self.filename = filename
        self.type = type
        self.time = time
        self.date = date
        self.cluster = cluster
        self.size = size

    def pack(self):
        # 确保文件名长度为11个字节
        packed_filename = self.filename.ljust(11).encode()
        # 使用正确的格式字符串进行打包
        packed_info = struct.pack('<B10xHHHI', self.type, self.time, self.date, self.cluster, self.size)
        return packed_filename + packed_info
    
    def get_date(self):
        year = ((self.date >> 9) & 0x7F) + 1980
        month = (self.date >> 5) & 0x0F
        day = self.date & 0x1F
        return year, month, day

    def get_time(self):
        hour = (self.time >> 11) & 0x1F
        minute = (self.time >> 5) & 0x3F
        second = (self.time & 0x1F) * 2
        return hour, minute, second

class DirInfo:
    def __init__(self, filename, start, count):
        self.filename = filename
        self.start = start
        self.count = count
        self.fileinfos = []

        with open(filename, 'rb') as f:
            f.seek(start)
            for _ in range(count):
                bin_data = f.read(FILEINFO_BYTES)
                if not bin_data or bin_data[0] == 0:
                    self.fileinfos.append(None)
                elif bin_data[0] == 0xE5:
                    self.fileinfos.append(False)
                else:
                    filename = bin_data[:11].decode().rstrip()
                    type, time, date, cluster, size = struct.unpack('<B10xHHHI', bin_data[11:])
                    self.fileinfos.append(FileInfo(filename, type, time, date, cluster, size))

    def write(self):
        with open(self.filename, 'r+b') as f:
            f.seek(self.start)
            for fileinfo in self.fileinfos:
                if fileinfo is None:
                    break
                if fileinfo:
                    f.write(fileinfo.pack())
                else:
                    f.write(b'\xE5' + b'\x00' * (FILEINFO_BYTES - 1))

    def find_free_index(self, start=0):
        for i in range(start, len(self.fileinfos)):
            if not self.fileinfos[i]:
                return i
        return False

    def add(self, index, filename, size, cluster):
        date, time = calc_timestamp(datetime.now())
        self.fileinfos[index] = FileInfo(filename, FILE_NORMAL, time, date, cluster, size)

    def find(self, filename):
        for i, fileinfo in enumerate(self.fileinfos):
            if fileinfo and fileinfo.filename == filename:
                return i
        return False

    def del_file(self, filename):
        index = self.find(filename)
        if index is not False:
            fileinfo = self.fileinfos[index]
            self.fileinfos[index] = False
            return fileinfo.cluster, fileinfo.size
        return None, None


def format_disk(filename):
    with open(filename, 'wb') as f:
        img = bytearray([0] * FD_SIZE)
        # 设置FAT表的初始值
        img[FAT1_ADDRESS:FAT1_ADDRESS + 3] = img[FAT2_ADDRESS:FAT2_ADDRESS + 3] = bytearray([0xF0, 0xFF, 0xFF])
        f.write(img)


def write_cluster(f, cluster_no, data):
    f.seek(ENTITY_ADDRESS + cluster_no * CLUSTER_BYTES)
    f.write(data[:CLUSTER_BYTES])


def save(filename, target_fn, fat, dirinfo):
    if not os.path.exists(target_fn):
        return False

    size = os.path.getsize(target_fn)
    cluster_count = (size + CLUSTER_BYTES - 1) // CLUSTER_BYTES
    start_cluster = fat.have_space(cluster_count)
    if start_cluster is False:
        return False

    file_index = dirinfo.find_free_index()
    if file_index is False:
        return False

    with open(target_fn, 'rb') as file_data:
        data = file_data.read()

    # 写入文件信息
    dirinfo.add(file_index, filename83(target_fn), size, start_cluster)

    # 写入数据和FAT
    prev_cluster = -1
    with open(filename, 'r+b') as f:
        while size > 0:
            cluster = fat.find_free_cluster(prev_cluster + 1)
            if cluster is False:
                return False  # 没有足够的空闲簇

            if prev_cluster >= 0:
                fat.put_next(prev_cluster, cluster)

            write_cluster(f, cluster, data[:CLUSTER_BYTES])
            data = data[CLUSTER_BYTES:]
            size -= CLUSTER_BYTES

            if size <= 0:
                fat.put_next(cluster, 0xFFF)  # 标记链的结束
            prev_cluster = cluster

    return True



def delete(image_fn, target_fn):
    dirinfo = DirInfo(image_fn, DIR_ADDRESS, FILE_COUNT)
    cluster, size = dirinfo.del_file(filename83(target_fn))

    if cluster is None:
        return False  # 文件未找到

    fat = Fat(image_fn)
    fat.del_chain(cluster, size)

    # 更新目录和FAT
    dirinfo.write()
    fat.write()

    return True


def load(image_fn, target_fn, fat):
    dirinfo = DirInfo(image_fn, DIR_ADDRESS, FILE_COUNT)
    file_info = dirinfo.find(filename83(target_fn))

    if file_info is False:
        return False  # 文件未找到

    with open(image_fn, 'rb') as f:
        cluster = file_info.cluster
        size = file_info.size
        data = bytearray()

        while size > 0:
            f.seek(ENTITY_ADDRESS + cluster * CLUSTER_BYTES)
            read_size = min(size, CLUSTER_BYTES)
            data.extend(f.read(read_size))

            size -= read_size
            cluster = fat.get_next(cluster)

    # 保存或处理数据
    # 示例：保存到文件
    with open(target_fn, 'wb') as out_file:
        out_file.write(data)

    return True

def write_data(image_fn, target_fn, start_sector):
    if not os.path.exists(target_fn):
        print(f"Target file {target_fn} does not exist.")
        return False

    SECTOR_SIZE = 512  # 每个扇区的大小

    with open(target_fn, 'rb') as file_data, open(image_fn, 'r+b') as img_file:
        # 读取要写入的整个文件数据
        data = file_data.read()
        total_sectors = (len(data) + SECTOR_SIZE - 1) // SECTOR_SIZE

        for i in range(total_sectors):
            offset = (start_sector + i) * SECTOR_SIZE
            img_file.seek(offset)
            img_file.write(data[i * SECTOR_SIZE:(i + 1) * SECTOR_SIZE])

    return True


def main():
    if len(sys.argv) < 3:
        print("Usage: python fat12img.py [image file name] [command] [arguments...]")
        sys.exit(1)

    image_fn = sys.argv[1]
    command = sys.argv[2]

    if command == 'format':
        format_disk(image_fn)

    elif command == 'dir':
        dirinfo = DirInfo(image_fn, DIR_ADDRESS, FILE_COUNT)
        for i in range(dirinfo.count):
            file_info = dirinfo.get(i)
            if file_info is None:
                break
            if file_info:
                year, month, day = file_info.get_date()
                hour, minute, second = file_info.get_time()
                print(f"{file_info.filename} {file_info.size} bytes {year}/{month}/{day} {hour}:{minute}:{second}")

    elif command == 'save':
        if len(sys.argv) < 4:
            print("Usage: python fat12img.py [image file name] save [target file name]")
            sys.exit(1)
        target_fn = sys.argv[3]
        fat = Fat(image_fn)
        dirinfo = DirInfo(image_fn, DIR_ADDRESS, FILE_COUNT)
        if not save(image_fn, target_fn, fat, dirinfo):
            print(f"Failed to save {target_fn}")
            sys.exit(1)
        dirinfo.write()
        fat.write()

    elif command == 'load':
        if len(sys.argv) < 4:
            print("Usage: python fat12img.py [image file name] load [target file name]")
            sys.exit(1)
        target_fn = sys.argv[3]
        fat = Fat(image_fn)
        if not load(image_fn, target_fn, fat):
            print(f"Failed to load {target_fn}")
            sys.exit(1)

    elif command == 'del':
        if len(sys.argv) < 4:
            print("Usage: python fat12img.py [image file name] del [target file name]")
            sys.exit(1)
        target_fn = sys.argv[3]
        if not delete(image_fn, target_fn):
            print(f"Failed to delete {target_fn}")
            sys.exit(1)

    elif command == 'write':
        if len(sys.argv) < 5:
            print("Usage: python fat12img.py [image file name] write [target file name] [cluster]")
            sys.exit(1)
        target_fn = sys.argv[3]
        cluster = int(sys.argv[4])
        write_data(image_fn, target_fn, cluster)

    else:
        print("Invalid command")
        sys.exit(1)

if __name__ == '__main__':
    main()
