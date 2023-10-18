#ifndef DEBUG_H
#define DEBUG_H

class DebugData {
public:
  int triangles = 0;
  int indices = 0;

  void addTriangles(int num) {
    triangles += num;
    indices += num * 3;
  }

  void clear() {
    triangles = 0;
    indices = 0;
  }

private:
};

extern DebugData debugData;

#endif