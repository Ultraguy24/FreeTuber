# FreeTuber
Free, open-source VTubing toolkit in modern C++. Linux-first.

## Features
- OpenGL renderer
- VRM Support
- Head Tracking

## Run
```bash
./FreeTuber <path/to/model.vrm>```

# Build
git clone https://github.com/ultraguy24/FreeTuber.git
cd FreeTuber
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja

# Credits
FreeTuber Developer - Ultraguy24
TinyGLTF - Syoyo Fujita, Aurélien Chatelain and many contributors
