/*
 * curl_wrapper.cc
 *
 *  Created on: Oct 29, 2021
 *      Author: bach
 */

#include "curl_wrapper.h"

// This must be defined in your .cpp file if you use this and split it into header
// and implementation parts.
EasyCurly::CurlGlobalInit EasyCurly::setup_and_teardown{};
