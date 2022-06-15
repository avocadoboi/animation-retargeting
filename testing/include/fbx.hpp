#ifndef ANIMATION_RETARGETING_TESTING_FBX_HPP
#define ANIMATION_RETARGETING_TESTING_FBX_HPP

#include <fbxsdk.h>

namespace testing {

namespace fbx {

template<typename T>
class Unique {
private:
    T* object_{};

public:
    Unique(T* const object) :
        object_{object}
    {
    }
    ~Unique() {
        if (object_) {
            object_->Destroy();
        }
    }

    Unique(Unique&& other) :
        object_{other.object_}
    {
        other.object_ = nullptr;
    }
    Unique const& operator=(Unique&& other) {
        if (object_) {
            object_->Destroy();
        }
        object_ = other.object_;
        other.object_ = nullptr;
        return *this;
    }

    Unique(Unique const&) = delete;
    Unique const& operator=(Unique const&) = delete;

    T const* get() const {
        return object_;
    }
    T* get() {
        return object_;
    }

    T const* operator->() const {
        return object_;
    }
    T* operator->() {
        return object_;
    }
};

template<typename T, typename ... Args>
Unique<T> create(Args&& ... args) {
    return Unique<T>{T::Create(std::forward<Args>(args)...)};
}

} // namespace fbx

} // namespace testing

#endif
