#ifndef ANIMATION_RETARGETING_TESTING_SKELETON_HPP
#define ANIMATION_RETARGETING_TESTING_SKELETON_HPP

#include "fbx.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>

namespace testing {
    
struct Bone {
    using Id = unsigned int;

    Bone const* parent;
    std::string name;
};

class Skeleton {
private:
    std::vector<Bone> bones_;
    
    FbxNode const* find_root_bone_(FbxNode const* const root_node)
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
            if (auto const* const node = find_root_bone_(root_node->GetChild(i)))
            {
                return node;
            }
        }

        return nullptr;
    }

    void add_bone_(FbxNode const* const bone, Bone const* parent)
    {
        bones_.push_back(Bone{parent, bone->GetName()});

        parent = &bones_.back();

        for (auto i = int{}; i < bone->GetChildCount(); ++i)
        {
            add_bone_(bone->GetChild(i), parent);
        }    
    }
    
    auto bone_iterator_by_name_(std::string const& name) const
    {
        return std::find_if(bones_.begin(), bones_.end(), [&](Bone const& bone) { return bone.name == name; });
    }
    
public:
    Skeleton() {
        bones_.reserve(256);
    }

    void load_from_fbx_node(FbxNode const* const root_node) 
    {
        if (auto const* const root_bone = find_root_bone_(root_node))
        {
            add_bone_(root_bone, nullptr);
        }
    }

    Bone const* bone_by_name(std::string const& name) const
    {
        auto const pos = bone_iterator_by_name_(name);
        if (pos != bones_.end())
        {
            return &*pos;
        }
        return nullptr;
    }
    Bone::Id bone_id_by_name(std::string const& name) const
    {
        return static_cast<Bone::Id>(bone_iterator_by_name_(name) - bones_.begin());
    }
    Bone const* bone_by_id(Bone::Id const id) const
    {
        return &bones_[id];
    }

    std::size_t bone_count() const {
        return bones_.size();
    }
};

} // namespace testing

#endif