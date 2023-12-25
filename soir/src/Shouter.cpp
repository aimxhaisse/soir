#include "Shouter.hh"

#include <glog/logging.h>

namespace maethstro {

Status Shouter::Init(const Config& config) {
  LOG(INFO) << "initializing connection to liquidsoap";

  if (!(shouter_ = shout_new())) {
    RETURN_ERROR(INTERNAL_ERROR, "could not allocate shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting hostname of shouter to " << config.liquidHost_.c_str();
  if (shout_set_host(shouter_, config.liquidHost_.c_str()) !=
      SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set hostname of shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting protocol of shouter to " << SHOUT_PROTOCOL_HTTP;
  if (shout_set_protocol(shouter_, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set shouter protocol, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting port of shouter to " << config.liquidPort_;
  if (shout_set_port(shouter_, config.liquidPort_) != SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set port of shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting mount of shouter to " << config.liquidMount_;
  if (shout_set_mount(shouter_, config.liquidMount_.c_str()) !=
      SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set mount of shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting format of shouter to " << SHOUT_FORMAT_OGG;
  if (shout_set_format(shouter_, SHOUT_FORMAT_OGG) != SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set format of shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "setting password of shouter to " << config.liquidPassword_;
  if (shout_set_password(shouter_, config.liquidPassword_.c_str()) !=
      SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "could not set password of shouter, err="
                                     << shout_get_error(shouter_));
  }

  LOG(INFO) << "opening shouter connection";
  if (shout_open(shouter_) != SHOUTERR_SUCCESS) {
    LOG(WARNING) << "failed to open connection to liquidsoap, err="
                 << shout_get_error(shouter_);

    RETURN_ERROR(INTERNAL_ERROR, "unable to open connection to liquidsoap, err="
                                     << shout_get_error(shouter_));
  }

  return StatusCode::OK;
}

Status Shouter::Send(const void* data, std::size_t size) {
  LOG(INFO) << "sending " << size << " bytes to liquidsoap";
  if (shout_send(shouter_, static_cast<const unsigned char*>(data), size) !=
      SHOUTERR_SUCCESS) {
    RETURN_ERROR(INTERNAL_ERROR, "unable to send data to shoutcast, err="
                                     << shout_get_error(shouter_));
  }

  shout_sync(shouter_);

  return StatusCode::OK;
}

void Shouter::Close() {
  shout_close(shouter_);
  shout_shutdown();
}

}  // namespace maethstro
