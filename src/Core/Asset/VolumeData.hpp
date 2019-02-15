#pragma once

#include <Core/Asset/AssetData.hpp>

namespace Ra {
namespace Core {
namespace Geometry {
class AbstractVolume;
} // namespace Geometry

namespace Asset {

struct VolumeData : public AssetData {
    inline VolumeData( const std::string& name = "" ) : AssetData( name ) {}
    Geometry::AbstractVolume* volume;
};

} // namespace Asset
} // namespace Core
} // namespace Ra
