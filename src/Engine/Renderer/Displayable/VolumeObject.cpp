#include <Core/Geometry/Volume.hpp>

#include <Engine/Renderer/Displayable/VolumeObject.hpp>
#include <Engine/Renderer/OpenGL/OpenGL.hpp>

#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Types.hpp>

namespace Ra {
namespace Engine {

VolumeObject::VolumeObject( const std::string& name ) :
    Displayable( name ),
    m_tex( TextureParameters() ),
    m_mesh( name + "_internal" ) {

    using Core::Aabb;

    m_tex.m_textureParameters.target = GL_TEXTURE_3D;
    m_tex.m_textureParameters.format = GL_RED;
    m_tex.m_textureParameters.internalFormat = GL_RED_INTEGER;
    m_tex.m_textureParameters.type = GL_INT;
}

VolumeObject::~VolumeObject() {}

void VolumeObject::loadGeometry( Core::Geometry::AbstractVolume* volume ) {
    if ( volume != nullptr && volume->isDense() )
    {

        m_mesh.loadGeometry( Core::Geometry::makeSharpBox( volume->computeAabb() ) );

        Core::Geometry::VolumeGrid* grid = static_cast<Core::Geometry::VolumeGrid*>( volume );
        m_volume = std::unique_ptr<Core::Geometry::AbstractVolume>( volume );
        m_tex.m_textureParameters.texels = const_cast<float*>( &( grid->data()[0] ) );
        m_isDirty = true;
    }
}

void VolumeObject::updateGL() {
    if ( m_isDirty )
    {
        m_mesh.updateGL();
        GL_CHECK_ERROR;
        // m_tex.initializeGL();
        GL_CHECK_ERROR;
        m_isDirty = false;
    }
}

void VolumeObject::render() {
    GL_CHECK_ERROR;
    m_mesh.render();
    GL_CHECK_ERROR;
}

} // namespace Engine
} // namespace Ra
