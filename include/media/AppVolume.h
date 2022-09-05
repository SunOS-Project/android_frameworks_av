/*
 * Copyright (C) 2022 Project Kaleidoscope
 * Copyright (C) 2023 Nameless-AOSP
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

#ifndef APP_VOLUME_H
#define APP_VOLUME_H

#include <android/media/AppVolumeData.h>
#include <utils/String8.h>
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <media/AidlConversionUtil.h>

namespace android {
namespace media {
    class AppVolume : public Parcelable {
    public:
        String8 packageName;
        int uid;
        float volume;
        bool muted;
        bool active;

        bool operator <(const AppVolume &obj) const {
            if (active != obj.active) return active < obj.active;
            if (packageName != obj.packageName) return packageName < obj.packageName;
            return uid < obj.uid;
        }

        virtual status_t writeToParcel(Parcel* parcel) const {
            AppVolumeData parcelable;
            return writeToParcelable(&parcelable)
                ?: parcelable.writeToParcel(parcel);
        }

        virtual status_t writeToParcelable(AppVolumeData* parcelable) const {
            parcelable->packageName = packageName.c_str();
            parcelable->uid = uid;
            parcelable->volume = volume;
            parcelable->muted = muted;
            parcelable->active = active;
            return OK;
        }

        virtual status_t readFromParcel(const Parcel* parcel) {
            AppVolumeData data;
            return data.readFromParcel(parcel)
                ?: readFromParcelable(data);
        }

        virtual status_t readFromParcelable(const AppVolumeData& parcelable) {
            packageName = parcelable.packageName.c_str();
            uid = parcelable.uid;
            volume = parcelable.volume;
            muted = parcelable.muted;
            active = parcelable.active;
            return OK;
        }
    };

    inline ConversionResult<AppVolume>
    aidl2legacy_AppVolume(const AppVolumeData& aidl) {
        AppVolume legacy;
        RETURN_IF_ERROR(legacy.readFromParcelable(aidl));
        return legacy;
    }

    inline ConversionResult<AppVolumeData>
    legacy2aidl_AppVolume(const AppVolume& legacy) {
        AppVolumeData aidl;
        RETURN_IF_ERROR(legacy.writeToParcelable(&aidl));
        return aidl;
    }
}   // namespace media
};  // namespace android

#endif // APP_VOLUME_H
