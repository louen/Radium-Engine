#include <Core/Geometry/Volume.hpp>

#include <Engine/Renderer/Displayable/VolumeObject.hpp>
#include <Engine/Renderer/OpenGL/OpenGL.hpp>

namespace Ra {
namespace Engine {

VolumeObject::VolumeObject( const std::string& name ) : Displayable( name ) {}

VolumeObject::~VolumeObject() {}

void VolumeObject::loadGeometry( Core::Geometry::AbstractVolume* volume ) {
    m_volume = std::unique_ptr<Core::Geometry::AbstractVolume>( volume );
}

void VolumeObject::updateGL() {
    if ( m_isDirty )
    {

        GL_CHECK_ERROR;
        m_isDirty = false;
    }
}

void VolumeObject::render() {
    GL_CHECK_ERROR;
}

} // namespace Engine
} // namespace Ra
