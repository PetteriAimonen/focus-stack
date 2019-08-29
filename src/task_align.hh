// Aligns one image against a reference image.
// Algorithm is based on "A Pyramid Approach to Subpixel Registration Based on Intensity" by
//                       P. Thévenaz, U.E. Ruttimann, M. Unser, January 1998

#pragma once
#include "worker.hh"

namespace focusstack {

class Task_Align: public ImgTask
{
public:
  Task_Align(ImgTask &reference, ImgTask &img);

  virtual void run();

  virtual std::string filename() const { return m_img.filename(); }
  virtual std::string name() const { return "Align " + filename(); }

private:
  ImgTask &m_reference;
  ImgTask &m_img;
};

}
