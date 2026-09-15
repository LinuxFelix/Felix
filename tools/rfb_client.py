"""Small RFB test client; no third-party Python packages required."""
import socket
import struct
import zlib


class Viewer:
    def __init__(self, host="127.0.0.1", port=5900, version=8):
        self.sock = socket.create_connection((host, port), timeout=5)
        self.sock.settimeout(5)
        assert self.read(12) == b"RFB 003.008\n"
        self.sock.sendall(f"RFB 003.{version:03d}\n".encode())
        if version == 3:
            assert self.read(4) == b"\0\0\0\1"
        else:
            count = self.read(1)[0]
            assert 1 in self.read(count)
            self.sock.sendall(b"\1")
            if version == 8:
                assert self.read(4) == b"\0" * 4
        self.sock.sendall(b"\1")
        header = self.read(24)
        self.width, self.height = struct.unpack(">HH", header[:4])
        self.name = self.read(struct.unpack(">I", header[20:])[0]).decode()
        self.sock.sendall(struct.pack('>BxxxBBBBHHHBBBxxx',0,32,24,0,1,255,255,255,16,8,0))
        self.sock.sendall(struct.pack(">BBHi", 2, 0, 1, 0))

    def read(self, length):
        data = bytearray()
        while len(data) < length:
            part = self.sock.recv(length - len(data))
            if not part:
                raise EOFError("RFB peer disconnected")
            data.extend(part)
        return bytes(data)

    def frame(self):
        self.sock.sendall(struct.pack(">BBHHHH", 3, 0, 0, 0, self.width, self.height))
        kind=self.read(1)
        while kind==b'\x02': kind=self.read(1)  # bell
        assert kind==b'\0'
        count=struct.unpack('>xH',self.read(3))[0]
        pixels=bytearray(self.width*self.height*4)
        for _ in range(count):
            x,y,w,h,encoding=struct.unpack('>HHHHi',self.read(12))
            assert encoding==0 and x+w<=self.width and y+h<=self.height
            data=self.read(w*h*4)
            for row in range(h):
                offset=((y+row)*self.width+x)*4
                pixels[offset:offset+w*4]=data[row*w*4:(row+1)*w*4]
        return bytes(pixels)

    def pointer(self, buttons, x, y):
        self.sock.sendall(struct.pack(">BBHH", 5, buttons, x, y))

    def key(self, keysym, down):
        self.sock.sendall(struct.pack(">BBHI", 4, int(down), 0, keysym))

    def screenshot(self, path):
        pixels = self.frame()
        rgb = bytearray(self.width * self.height * 3)
        rgb[0::3], rgb[1::3], rgb[2::3] = pixels[2::4], pixels[1::4], pixels[0::4]
        with open(path, "wb") as stream:
            if str(path).endswith(".png"):
                def chunk(kind, data):
                    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
                stream.write(b"\x89PNG\r\n\x1a\n")
                stream.write(chunk(b"IHDR", struct.pack(">IIBBBBB", self.width, self.height, 8, 2, 0, 0, 0)))
                rows = b"".join(b"\0" + rgb[y*self.width*3:(y+1)*self.width*3] for y in range(self.height))
                stream.write(chunk(b"IDAT", zlib.compress(rows)))
                stream.write(chunk(b"IEND", b""))
            else:
                stream.write(f"P6\n{self.width} {self.height}\n255\n".encode())
                stream.write(rgb)

    def close(self):
        self.sock.close()


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=5900)
    parser.add_argument("--output", default="felix.ppm")
    args = parser.parse_args()
    viewer = Viewer(port=args.port)
    viewer.screenshot(args.output)
    print(f"{viewer.name}: {viewer.width}x{viewer.height}; saved {args.output}")
    viewer.close()
