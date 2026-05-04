#pragma once

#include "Asset.hpp"
#include "Mesh.hpp"
#include <vector>

namespace platypus
{
    class Model : public Asset
    {
    private:
        std::string _filepath;
        // NOTE: Not sure if "assets inside assets" should rather be IDs to those instead of ptrs!
        //  -> ptrs will become issue when serializing
        std::vector<Mesh*> _meshes;

        // NOTE:
        // instanced property should be per mesh and NOT per model, this is just a quick hack fix
        // for serialization.
        // TODO: Fix everywhere!
        bool _instanced = false;

    public:
        // NOTE: Meshes ownership doesn't transfer here!
        Model(
            const std::string& filepath,
            bool instanced,
            const std::vector<Mesh*>& meshes,
            const std::string& name = "",
            ID_t id = NULL_ID
        );
        ~Model();

        inline const std::string& getFilepath() const { return _filepath; }
        inline bool isInstanced() const { return _instanced; }
        inline const std::vector<Mesh*>& getMeshes() const { return _meshes; }
    };
}
