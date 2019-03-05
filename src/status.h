#ifndef SOIR_STATUS_H
#define SOIR_STATUS_H

#include <cassert>
#include <iosfwd>
#include <string>

namespace soir {

// List of all status code available in Soir.
enum StatusCode {
  OK = 0,
  INVALID_CONFIG_FILE,
  INTERNAL_ERROR,
};

// Helper class to wrap status codes and attach messages to it. This
// is meant to be used everywhere we need to possibly return an error.
// This class is inspired from protobuf's status interface.
class Status {
public:
  Status();
  Status(StatusCode code, const std::string &message);
  Status(StatusCode code);
  Status(const Status &status);

  Status &operator=(const Status &status);
  bool operator==(StatusCode code) const;
  bool operator!=(StatusCode code) const;

  StatusCode Code() const;
  const std::string &Message() const;

private:
  StatusCode code_;
  std::string message_;
};

// Helper class to attach an object to a Status. The object is valid
// only if the status code is OK.
template <typename T> class StatusOr {
public:
  StatusOr() : status_(StatusCode::INTERNAL_ERROR), value_() {}
  StatusOr(StatusCode code) : status_(code), value_() {}
  StatusOr(const Status &status) : status_(status), value_() {}
  StatusOr(const T &value) : status_(StatusCode::OK), value_(value) {}
  StatusOr(T &&value) : status_(StatusCode::OK), value_(std::move(value)) {}

  const Status &GetStatus() const { return status_; }

  // Whether or not the status is OK.
  bool Ok() const { return status_ == StatusCode::OK; }

  // This will crash if the status is not OK.
  T &ValueOrDie() {
    assert(status_ == StatusCode::OK);
    return value_;
  }

private:
  Status status_;
  T value_;
};

std::ostream &operator<<(std::ostream &os, const Status &status);

} // namespace soir

#endif // SOIR_STATUS_H
