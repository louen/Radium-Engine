#include <Engine/Renderer/Displayable/VolumeObject.hpp>

namespace Ra {
namespace Engine {

const Core::Geometry::AbstractGeometry& VolumeObject::getGeometry() const {
    return *(Core::Geometry::AbstractGeometry*)( m_volume.get() );
}

Core::Geometry::AbstractGeometry& VolumeObject::getGeometry() {
    return *(Core::Geometry::AbstractGeometry*)( m_volume.get() );
}

/// Returns the underlying AbstractVolume
const Core::Geometry::AbstractVolume& VolumeObject::getVolume() const {
    return *m_volume.get();
}

Core::Geometry::AbstractVolume& VolumeObject::getVolume() {
    return *m_volume.get();
}

} // namespace Engine
} // namespace Ra
