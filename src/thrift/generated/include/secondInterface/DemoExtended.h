// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_DemoExtended
#define YARP_THRIFT_GENERATOR_DemoExtended

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>
#include <secondInterface/demo_common.h>
#include <secondInterface/Demo.h>

namespace yarp {
  namespace test {
    class DemoExtended;
  }
}


class yarp::test::DemoExtended :  public Demo {
public:
  DemoExtended() { yarp().setOwner(*this); }
  virtual Point3D multiply_point(const Point3D& x, const double factor);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

#endif

