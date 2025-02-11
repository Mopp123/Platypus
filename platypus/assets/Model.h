#pragma once

#include "Asset.h"
#include "Mesh.h"
#include <vector>

namespace platypus
{
    class Model : public Asset
    {
    private:
        // NOTE: Not sure if "assets inside assets" should rather be IDs to those instead of ptrs!
        //  -> ptrs will become issue when serializing!
        std::vector<Mesh*> _meshes;

    public:
        // NOTE: Meshes ownership doesn't transfer here!
        Model(const std::vector<Mesh*>& meshes);
        ~Model();

        inline const std::vector<Mesh*>& getMeshes() const { return _meshes; }
    };
}
