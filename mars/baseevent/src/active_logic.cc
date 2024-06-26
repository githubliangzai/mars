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
 * active_logic.cc
 *
 *  Created on: 2012-8-22
 *      Author: yerungui
 */

#include "mars/baseevent/active_logic.h"

#include "boost/bind.hpp"
#include "mars/baseevent/baseprjevent.h"
#include "mars/comm/bootrun.h"
#include "mars/comm/messagequeue/message_queue.h"
#include "mars/comm/singleton.h"
#include "mars/comm/thread/lock.h"
#include "mars/comm/thread/mutex.h"
#include "mars/comm/time_utils.h"
#include "mars/comm/xlogger/xlogger.h"

namespace mars {
namespace comm {

static void onForeground(bool _isforeground) {
    ActiveLogic::Instance()->OnForeground(_isforeground);
}

static void __initbind_baseprjevent() {
    GetSignalOnForeground().connect(&onForeground);
}

BOOT_RUN_STARTUP(__initbind_baseprjevent);

#define INACTIVE_TIMEOUT (10 * 60 * 1000)  // ms

#ifdef ANDROID
static void onAlarm(int64_t id) {
    Alarm::onAlarmImpl(id);
}
static const int kAlarmType = 100;
#endif

std::shared_ptr<ActiveLogic> ActiveLogic::inst_ = nullptr;
std::shared_ptr<ActiveLogic> ActiveLogic::Instance() {
    static Mutex mtx;
    if (!inst_) {
        ScopedLock lock(mtx);
        if (!inst_) {
            inst_ = std::make_shared<ActiveLogic>();
        }
    }

    return inst_;
}

void ActiveLogic::Release() {
    inst_ = nullptr;
}

ActiveLogic::ActiveLogic()
: isforeground_(false)
, isactive_(true)
, alarm_(boost::bind(&ActiveLogic::__OnInActive, this), false)
, lastforegroundchangetime_(::gettickcount()) {
    xinfo_function(TSF "MQ:%_, this:%_", MessageQueue::GetDefMessageQueue(), this);

#ifdef __ANDROID__
    GetSignalOnAlarm().connect(&onAlarm);
    alarm_.SetType(kAlarmType);
#endif
#ifndef __APPLE__
    if (!alarm_.Start(INACTIVE_TIMEOUT)) {
        xerror2(TSF "m_alarm.Start false");
    }
#endif
}

ActiveLogic::~ActiveLogic() {
    xinfo_function();
    MessageQueue::CancelMessage(MessageQueue::DefAsyncInvokeHandler(MessageQueue::GetDefMessageQueue()),
                                (MessageQueue::MessageTitle_t)this);
    MessageQueue::WaitForRunningLockEnd(MessageQueue::DefAsyncInvokeHandler(MessageQueue::GetDefMessageQueue()));
}

void ActiveLogic::OnForeground(bool _isforeground) {
    if (MessageQueue::GetDefMessageQueue() != MessageQueue::CurrentThreadMessageQueue()) {
        MessageQueue::AsyncInvoke(boost::bind(&ActiveLogic::OnForeground, this, _isforeground),
                                  (MessageQueue::MessageTitle_t)this,
                                  mq::DefAsyncInvokeHandler(mq::GetDefMessageQueue()),
                                  "ActiveLogic::OnForeground");
        return;
    }

    xgroup2_define(group);
    xinfo2(TSF "OnForeground:%0, change:%1, this:%2", _isforeground, _isforeground != isforeground_, this) >> group;

    if (_isforeground == isforeground_)
        return;

    bool oldisactive = isactive_;
    isactive_ = true;
    isforeground_ = _isforeground;
    lastforegroundchangetime_ = ::gettickcount();
    alarm_.Cancel();

    if (!isforeground_) {
#ifndef __APPLE__
        if (!alarm_.Start(INACTIVE_TIMEOUT)) {
            xerror2(TSF "m_alarm.Start false") >> group;
        }
#endif
    }

    bool isnotify = oldisactive != isactive_;
    SignalForeground(isforeground_);

    if (isnotify) {
        xinfo2(TSF "active change:%0", isactive_) >> group;
        SignalActive(isactive_);
    }
}

bool ActiveLogic::IsActive() const {
    return isactive_;
}

bool ActiveLogic::IsForeground() const {
    return isforeground_;
}

uint64_t ActiveLogic::LastForegroundChangeTime() const {
    return lastforegroundchangetime_;
}

void ActiveLogic::__OnInActive() {
    xdebug_function();
    if (!isforeground_)
        isactive_ = false;

    bool isactive = isactive_;
    xinfo2(TSF "active change:%0", isactive_);
    SignalActive(isactive);
}

void ActiveLogic::SwitchActiveStateForDebug(bool _active) {
    isactive_ = _active;
    __OnInActive();
}

}  // namespace comm
}  // namespace mars