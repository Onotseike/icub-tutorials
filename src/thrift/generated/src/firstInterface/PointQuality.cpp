// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>
#include <firstInterface/PointQuality.h>

namespace yarp { namespace test {


int PointQualityVocab::fromString(const std::string& input) {
  // definitely needs optimizing :-)
  if (input=="UNKNOWN") return (int)UNKNOWN;
  if (input=="GOOD") return (int)GOOD;
  if (input=="BAD") return (int)BAD;
  return -1;
}
std::string PointQualityVocab::toString(int input) {
  switch((PointQuality)input) {
  case UNKNOWN:
    return "UNKNOWN";
  case GOOD:
    return "GOOD";
  case BAD:
    return "BAD";
  }
  return "";
}
}} // namespace


