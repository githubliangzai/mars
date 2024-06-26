// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
 * basechecker.h
 *
 *  Created on: 2016/06/24
 *      Author: renlibin caoshaokun
 *  Copyright © 2014 Tencent. All rights reserved.
 */

#ifndef SDT_SRC_ACTIVECHECK_BASECHEKCER_H_
#define SDT_SRC_ACTIVECHECK_BASECHEKCER_H_

#include <string>
#include <vector>

#include "mars/comm/platform_comm.h"
#include "mars/comm/thread/mutex.h"
#include "mars/comm/thread/thread.h"
#include "mars/sdt/sdt.h"
#include "netchecker_profile.h"

namespace mars {
namespace sdt {

class BaseChecker {
 public:
    BaseChecker();
    virtual ~BaseChecker();

 public:
    virtual int StartDoCheck(CheckRequestProfile& _check_request) = 0;
    int CancelDoCheck();

 protected:
    virtual void __DoCheck(CheckRequestProfile& _check_request) = 0;

 protected:
    bool is_canceled_ = false;
};

}  // namespace sdt
}  // namespace mars

#endif  // SDT_SRC_ACTIVECHECK_BASECHEKCER_H_
