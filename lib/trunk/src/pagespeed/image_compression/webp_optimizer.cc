/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Victor Chudnovsky

#include "pagespeed/image_compression/webp_optimizer.h"

#include <cstdlib>

#include "base/basictypes.h"
#include "base/logging.h"

namespace pagespeed {

namespace image_compression {

// Copied from libwebp/v0_2/examples/cwebp.c
static const char* const kWebPErrorMessages[] = {
  "OK",
  "OUT_OF_MEMORY: Out of memory allocating objects",
  "BITSTREAM_OUT_OF_MEMORY: Out of memory re-allocating byte buffer",
  "NULL_PARAMETER: NULL parameter passed to function",
  "INVALID_CONFIGURATION: configuration is invalid",
  "BAD_DIMENSION: Bad picture dimension. Maximum width and height "
  "allowed is 16383 pixels.",
  "PARTITION0_OVERFLOW: Partition #0 is too big to fit 512k.\n"
  "To reduce the size of this partition, try using less segments "
  "with the -segments option, and eventually reduce the number of "
  "header bits using -partition_limit. More details are available "
  "in the manual (`man cwebp`)",
  "PARTITION_OVERFLOW: Partition is too big to fit 16M",
  "BAD_WRITE: Picture writer returned an I/O error",
  "FILE_TOO_BIG: File would be too big to fit in 4G",
  "USER_ABORT: encoding abort requested by user"
};

// Byte-emission function for use via WebPPicture.writer.
int WriteWebpIncrementally(const uint8_t* data, size_t data_size,
                           const WebPPicture* const pic) {
  std::string* const out = static_cast<std::string*>(pic->custom_ptr);
  out->append(reinterpret_cast<const char*>(data), data_size);
  DLOG(INFO) << "Writing to webp: " << data_size << " bytes."
             << "Total size: " << out->size();
  return true;
}

void WebpConfiguration::CopyTo(WebPConfig* webp_config) const {
  webp_config->lossless = lossless;
  webp_config->quality = quality;
  webp_config->method = method;
  webp_config->target_size = target_size;
  webp_config->alpha_compression = alpha_compression;
  webp_config->alpha_filtering = alpha_filtering;
  webp_config->alpha_quality = alpha_quality;
}

WebpScanlineWriter::WebpScanlineWriter()
    : stride_bytes_(0), rgb_(NULL), rgb_end_(NULL), position_bytes_(NULL),
      config_(NULL), webp_image_(NULL), has_alpha_(false),
      init_ok_(false), imported_(false), got_all_scanlines_(false),
      progress_hook_(NULL), progress_hook_data_(NULL) {
}

WebpScanlineWriter::~WebpScanlineWriter() {
  if (imported_) {
    WebPPictureFree(&picture_);
  }
  delete config_;
  free(rgb_);
}

bool WebpScanlineWriter::InitializeWrite(const WebpConfiguration& config,
                                         std::string* const out) {
  if (!init_ok_) {
    LOG(ERROR) << "Init() previously failed";
    return false;
  }

  // Since config_ might have been modified during a previous call to
  // FinalizeWrite() and can't be re-used, create a fresh copy.
  delete config_;
  config_ = new WebPConfig();
  if (!WebPConfigInit(config_)) {
    DLOG(INFO) << "WebPConfigInit failed";
    return false;
  }

  config.CopyTo(config_);

  if (!WebPValidateConfig(config_)) {
    DLOG(INFO) << "WebPValidateConfig failed";
    return false;
  }

  webp_image_ = out;
  if (config.progress_hook) {
    progress_hook_ = config.progress_hook;
    progress_hook_data_ = config.user_data;
  }
  return true;
}

int WebpScanlineWriter::ProgressHook(int percent, const WebPPicture* picture) {
  const WebpScanlineWriter* webp_writer =
      static_cast<WebpScanlineWriter*>(picture->user_data);
  return webp_writer->progress_hook_(percent, webp_writer->progress_hook_data_);
}

bool WebpScanlineWriter::Init(const size_t width, const size_t height,
                              PixelFormat pixel_format) {
  if (init_ok_) {
    LOG(ERROR) << "Attempting to re-initialize successfully initialized image";
    return false;
  }

  if ((height > WEBP_MAX_DIMENSION) ||
      (width > WEBP_MAX_DIMENSION)) {
    LOG(ERROR) << "Each image dimension (" << width << "x" << height
               << ") must be less than " << WEBP_MAX_DIMENSION;
    return false;
  }
  switch (pixel_format) {
    case RGB_888:
      has_alpha_ = false;
      break;
    case RGBA_8888:
      has_alpha_ = true;
      break;
    default:
      LOG(ERROR) << "Invalid pixel format "
                 << GetPixelFormatString(pixel_format);
      return false;
  }
  DLOG(INFO) << "Pixel format: "  << GetPixelFormatString(pixel_format);
  if (!WebPPictureInit(&picture_)) {
    DLOG(INFO) << "WebPPictureInit failed";
    return false;
  }

  picture_.width = width;
  picture_.height = height;
  picture_.use_argb = true;
#ifndef NDEBUG
  picture_.stats = &stats_;
#endif

  COMPILE_ASSERT(sizeof(*rgb_) == 1, Expected_size_of_one_byte);
  stride_bytes_ = (has_alpha_ ? 4 : 3) * picture_.width * sizeof(*rgb_);
  int size_bytes = stride_bytes_ * picture_.height;
  if (rgb_ != NULL) {
    LOG(ERROR) << "Already initialized webp";
    return false;
  }
  if ((rgb_ = static_cast<uint8_t*>(malloc(size_bytes))) == NULL) {
    LOG(ERROR) << "Could not allocate memory for webp";
    return false;
  }
  rgb_end_ = rgb_ + size_bytes;
  position_bytes_ = rgb_;
  init_ok_ = true;
  return true;
}

bool WebpScanlineWriter::WriteNextScanline(void *scanline_bytes) {
  if ((position_bytes_ == NULL) ||
      (position_bytes_ + stride_bytes_ > rgb_end_)) {
    LOG(ERROR) << "Attempting to write past allocated memory "
               << "(rgb_ == " << static_cast<void*>(rgb_)
               << "; position_bytes_ == " << static_cast<void*>(position_bytes_)
               << "; stride_bytes_ == " << stride_bytes_
               << "; rgb_end_ == " << static_cast<void*>(rgb_end_)
               << ")";
    return false;
  }
  memcpy(position_bytes_, scanline_bytes, stride_bytes_);
  position_bytes_ += stride_bytes_;
  return true;
}

bool WebpScanlineWriter::FinalizeWrite() {
  if (!got_all_scanlines_) {
    if (position_bytes_ != rgb_end_) {
      LOG(ERROR) << "Can't FinalizeWrite: Some scanlines were not written";
      return false;
    }
    got_all_scanlines_ = true;
    position_bytes_ = NULL;
  }

  bool ok = has_alpha_ ?
      (WebPPictureImportRGBA(&picture_, rgb_, stride_bytes_) != 0) :
      (WebPPictureImportRGB(&picture_, rgb_, stride_bytes_) != 0);

  if (!ok) {
    DLOG(INFO) << "Could not import RGB(A) picture data for webp";
    return false;
  }

  imported_ = true;

  picture_.writer = WriteWebpIncrementally;
  picture_.custom_ptr = webp_image_;
  if (progress_hook_) {
    picture_.progress_hook = ProgressHook;
    picture_.user_data = this;
  }
  if (!WebPEncode(config_, &picture_)) {
    DLOG(INFO) << "Could not encode webp data. "
               << "Error " << picture_.error_code
               << ": " << kWebPErrorMessages[picture_.error_code];
    return false;
  }

#ifndef NDEBUG
  DLOG(INFO) << "Stats: coded_size: " << stats_.coded_size
             << "; lossless_size: " << stats_.lossless_size
             << "; alpha size: " << stats_.alpha_data_size
             << "; layer size: " << stats_.layer_data_size;
#endif

  return true;
}

}  // namespace image_compression

}  // namespace pagespeed
