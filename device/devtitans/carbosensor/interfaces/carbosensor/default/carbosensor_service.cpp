#include "carbosensor_service.h"

namespace aidl::devtitans::carbosensor {
    ndk::ScopedAStatus CarboSensorService::connect(int32_t* _aidl_return) {
        *_aidl_return = this->carboSensor.connect();
        LOG(INFO) << "connect(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus CarboSensorService::getMonoxide(int32_t* _aidl_return) {
        *_aidl_return = this->carboSensor.getLuminosity();
        LOG(INFO) << "getLuminosity(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }
}