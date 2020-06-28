/*
* Copyright (c) 2018-present, aliminabc@gmail.com.
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "AlUTimeline.h"
#include "Thread.h"
#include "TimeUtils.h"

#define TAG "AlUTimeline"

AlUTimeline::AlUTimeline(const std::string alias) : Unit(alias) {
    al_reg_msg(MSG_TIMELINE_SET_HZ_IN_US, AlUTimeline::_onSetHzInUS);
    al_reg_msg(MSG_TIMELINE_SET_DURATION, AlUTimeline::_onSetDurationUS);
    al_reg_msg(MSG_TIMELINE_START, AlUTimeline::_onStart);
    al_reg_msg(MSG_TIMELINE_PAUSE, AlUTimeline::_onPause);
    pipe = std::shared_ptr<AlEventPipeline>(AlEventPipeline::create(TAG));
}

AlUTimeline::~AlUTimeline() {

}

bool AlUTimeline::onCreate(AlMessage *msg) {
    return true;
}

bool AlUTimeline::onDestroy(AlMessage *msg) {
    return true;
}

bool AlUTimeline::_onSetHzInUS(AlMessage *msg) {
//    hzInUS = msg->arg1;
    hzInUS = 23220;
    return true;
}

bool AlUTimeline::_onSetDurationUS(AlMessage *msg) {
    mDurationInUS = msg->arg2;
    AlLogI(TAG, "%" PRId64, mDurationInUS);
    return true;
}

bool AlUTimeline::_onStart(AlMessage *msg) {
    beating = true;
    _heartbeat();
    return true;
}

bool AlUTimeline::_onPause(AlMessage *msg) {
    beating = false;
    return true;
}

void AlUTimeline::_heartbeat() {
    if (!beating) {
        return;
    }
    if (mDurationInUS > 0) {
        auto *msg = AlMessage::obtain(MSG_TIMELINE_PROGRESS_NOTIFY, AlMessage::QUEUE_MODE_UNIQUE);
        msg->arg1 = mCurTimeInUS * 1e6 / mDurationInUS;
        msg->arg2 = mDurationInUS;
        this->postMessage(msg);
    }

    pipe->queueEvent([this]() {
        this->_sendBeat();
        auto sleepTime = this->mLastBeatTimestampInUS > 0 ? (this->hzInUS -
                                                             (TimeUtils::getCurrentTimeUS() -
                                                              this->mLastBeatTimestampInUS))
                                                          : this->hzInUS;
        AlEventPipeline::sleep(sleepTime);
        this->mLastBeatTimestampInUS = TimeUtils::getCurrentTimeUS();
        this->_heartbeat();
    });
}

void AlUTimeline::_sendBeat() {
    if (0 == this->mCurTimeInUS) {
        postMessage(AlMessage::obtain(MSG_TIMELINE_BEGIN, AlMessage::QUEUE_MODE_UNIQUE));
    }
    // 如果QUEUE_MODE_FIRST_ALWAYS会影响数据流传递，所以使用QUEUE_MODE_UNIQUE即可
    auto mode = AlMessage::QUEUE_MODE_UNIQUE;
    auto *msg = AlMessage::obtain(MSG_TIMELINE_HEARTBEAT, mode);
    msg->arg2 = this->mCurTimeInUS;
    this->postMessage(msg);
    this->mCurTimeInUS += this->hzInUS;
    if (this->mCurTimeInUS > this->mDurationInUS) {
        auto *msg1 = AlMessage::obtain(MSG_TIMELINE_END, AlMessage::QUEUE_MODE_UNIQUE);
        msg1->arg2 = this->mDurationInUS;
        postMessage(msg1);
        this->mCurTimeInUS = 0;
    }
}