#ifndef SOIR_STATUS_H
#define SOIR_STATUS_H

#include <iosfwd>
#include <string>

namespace soir {

// List of all status code available in Soir.
enum StatusCode {
  OK = 0,
};

// Helper class to wrap status codes and attach messages to it. This
// is meant to be used everywhere we need to possibly return an error.
// This class is inspired from protobuf's status interface.
class Status {
public:
  Status();
  Status(StatusCode code, const std::string &message);
  explicit Status(StatusCode code);

  bool operator==(StatusCode code) const;
  bool operator!=(StatusCode code) const;

  StatusCode Code() const;
  const std::string &Message() const;

private:
  const StatusCode code_;
  const std::string message_;
};

std::ostream &operator<<(std::ostream &os, const Status &status);

} // namespace soir

#endif // SOIR_STATUS_H
