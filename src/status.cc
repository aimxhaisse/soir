#include <ostream>

#include "status.h"

namespace soir {

Status::Status() : code_(StatusCode::OK) {}

Status::Status(StatusCode code, const std::string &message)
    : code_(code), message_(message) {}

Status::Status(StatusCode code) : code_(code) {}

bool Status::operator==(StatusCode code) const { return code_ == code; }

bool Status::operator!=(StatusCode code) const { return !operator==(code); }

StatusCode Status::Code() const { return code_; }

const std::string &Status::Message() const { return message_; }

std::ostream &operator<<(std::ostream &os, const Status &status) {
  std::string str_code;

  switch (status.Code()) {
  case OK:
    str_code = "OK";
    break;
  case INTERNAL_ERROR:
    str_code = "INTERNAL_ERROR";
    break;
  default:
    str_code = "UNKNOWN";
    break;
  }

  if (status == StatusCode::OK) {
    os << str_code;
  } else {
    os << str_code << " (" << status.Message() << ")";
  }

  return os;
}

} // namespace soir
