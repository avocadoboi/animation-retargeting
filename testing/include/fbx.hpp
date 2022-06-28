// A few FBX SDK abstractions.

#ifndef ANIMATION_RETARGETING_TESTING_FBX_HPP
#define ANIMATION_RETARGETING_TESTING_FBX_HPP

#include <fbxsdk.h>
#include <glm/glm.hpp>

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

inline Unique<FbxScene> import_scene(FbxManager* manager, char const* const fbx_path)
{
    auto importer = fbx::create<FbxImporter>(manager, "");

    if (!importer->Initialize(fbx_path, -1, manager->GetIOSettings()))
    {
        throw std::runtime_error{"Failed to initialize FBX importer."};
    }

    auto scene = fbx::create<FbxScene>(manager, "");
    importer->Import(scene.get());

    return scene;
}

inline glm::vec3 to_glm_vec(FbxVector4 const vector) {
    return {
        vector.mData[0],
        vector.mData[1],
        vector.mData[2],
    };
}
inline glm::vec2 to_glm_vec(FbxVector2 const vector) {
    return {
        vector.mData[0],
        vector.mData[1],
    };
}

template<typename T>
auto layer_element_at(FbxLayerElementTemplate<T> const* const layer, int const index)
{
    return to_glm_vec(layer->GetReferenceMode() == FbxLayerElement::EReferenceMode::eDirect 
        ? layer->GetDirectArray().GetAt(index) 
        : layer->GetDirectArray().GetAt(layer->GetIndexArray().GetAt(index)));
}

inline FbxNode const* find_root_bone(FbxNode const* const root_node)
{
    if (auto const* const attribute = root_node->GetNodeAttribute())
    {
        if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        {
            return root_node;
        }
    }
    
    for (auto i = int{}; i < root_node->GetChildCount(); ++i)
    {
        if (auto const* const node = find_root_bone(root_node->GetChild(i)))
        {
            return node;
        }
    }

    return nullptr;
}

} // namespace fbx

} // namespace testing

#endif
