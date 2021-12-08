#include "../src/tree.h"
#include <iostream>

struct Photon
{
  float data[3];
  int type;
  float direction;

  Photon() {}
  Photon(float x, float y, float z) {
    data[0] = x;
    data[1] = y;
    data[2] = z;
  }
  Photon(float x, float y, float z, int t, float d) {
    data[0] = x;
    data[1] = y;
    data[2] = z;
    type = t;
    direction = d;
  }

  float operator [] (int i) const {
    return data[i];
  }

  bool operator == (const Photon& p) const {
    return data[0] == p[0] && data[1] == p[1] && data[2] == p[2];
  }

  friend std::ostream& operator << (std::ostream& s, const Photon& p) {
    return s << '(' << p[0] << ", " << p[1] << ", " << p[2] << ')';
  }
};

int main() {
  typedef KD::Core<3, Photon> CORE;

  Photon min(0, 0, 0);
  Photon max(9, 9, 9);
  KD::Tree<CORE> kdtree(min, max);

  kdtree.insert(Photon(1, 2, 3, 1, 5.0f));
  kdtree.insert(Photon(8, 2, 5, 2, 5.0f));
  kdtree.insert(Photon(1, 0, 8, 3, 5.0f));
  kdtree.insert(Photon(3, 1, 4, 4, 5.0f));

  Photon nearest = kdtree.nearest(Photon(5, 5, 5));
  std::cout << nearest.type;
  return 0;
}
