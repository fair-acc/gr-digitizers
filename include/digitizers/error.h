/* -*- c++ -*- */
/*
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDE_DIGITIZERS_ERROR_H_
#define INCLUDE_DIGITIZERS_ERROR_H_

#include <digitizers/api.h>
#include <boost/circular_buffer.hpp>
#include <system_error>
#include <string>
#include <mutex>

namespace err {
    struct DIGITIZERS_API error_info_t
    {
      int64_t timestamp;
      std::error_code code;
    };

    class error_buffer_t
    {
      boost::circular_buffer<error_info_t> cb;
      std::mutex access;

    public:

      error_buffer_t(int n):
        cb(n), access()
      {
      }

      ~error_buffer_t()
      {
      }

      void push(error_info_t err)
      {
        std::unique_lock<std::mutex> lk(access);
        cb.push_back(err);
      }

      std::vector<error_info_t> get()
      {
        std::unique_lock<std::mutex> lk(access);
        std::vector<error_info_t> ret;
        ret.insert(ret.begin(), cb.begin(), cb.end());
        return ret;
      }
    };
}//namespace error
#endif /* INCLUDE_DIGITIZERS_ERROR_H_ */
