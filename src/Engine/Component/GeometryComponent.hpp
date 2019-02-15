#ifndef RADIUMENGINE_GEOMETRY_COMPONENT_HPP
#define RADIUMENGINE_GEOMETRY_COMPONENT_HPP

#include <Core/Asset/GeometryData.hpp>
#include <Core/Asset/VolumeData.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Geometry/Volume.hpp>
#include <Engine/Component/Component.hpp>

namespace Ra {
namespace Engine {
class Mesh;
class VolumeObject;
} // namespace Engine
} // namespace Ra

namespace Ra {
namespace Engine {
/*!
 * \brief Main class to convert Ra::Core::Asset::GeometryData to Ra::Engine::Mesh
 *
 * Exports access to the mesh geometry:
 *  - TriangleMesh: get, rw (set vertices, normals and triangles dirty)
 *  - Vertices: rw (if deformable)
 *  - normals: rw (if deformable)
 *  - triangles: rw (if deformable)
 */
class RA_ENGINE_API TriangleMeshComponent : public Component {
  public:
    TriangleMeshComponent( const std::string& name, Entity* entity,
                           const Ra::Core::Asset::GeometryData* data );
    ~TriangleMeshComponent() override;

    void initialize() override;

    /// Returns the current display geometry.
    const Ra::Core::Geometry::TriangleMesh& getMesh() const;

  public:
    // Component communication management
    void setupIO( const std::string& id );
    void setContentName( const std::string& name );
    void setDeformable( bool b );

    /// Returns the index of the associated RO (the display mesh)
    Ra::Core::Utils::Index getRenderObjectIndex() const;

  private:
    void generateTriangleMesh( const Ra::Core::Asset::GeometryData* data );

    const Ra::Core::Geometry::TriangleMesh* getMeshOutput() const;
    Ra::Core::Geometry::TriangleMesh* getMeshRw();
    Ra::Core::Vector3Array* getVerticesRw();
    Ra::Core::Vector3Array* getNormalsRw();
    Ra::Core::VectorArray<Ra::Core::Vector3ui>* getTrianglesRw();

    const Ra::Core::Utils::Index* roIndexRead() const;

  private:
    Ra::Core::Utils::Index m_meshIndex{};
    std::string m_contentName{};
    // directly hold a reference to the displayMesh to simplify accesses in handlers
    std::shared_ptr<Mesh> _displayMesh{nullptr};
};

/*!
 * \brief Main class to convert Ra::Core::Asset::GeometryData to Ra::Engine::Mesh
 *
 * Exports access to the volume:
 *  - VolumeObject: get, rw
 */
class RA_ENGINE_API VolumeComponent : public Component {
  public:
    VolumeComponent( const std::string& name, Entity* entity,
                     const Ra::Core::Asset::VolumeData* data );
    ~VolumeComponent() override;

    void initialize() override;

  public:
    // Component communication management
    void setupIO( const std::string& id );
    void setContentName( const std::string& name );

    /// Returns the index of the associated RO (the display mesh)
    Ra::Core::Utils::Index getRenderObjectIndex() const;

  private:
    void generateVolumeRender( const Ra::Core::Asset::VolumeData* data );

    const Ra::Core::Geometry::AbstractVolume* getVolumeOutput() const;
    Ra::Core::Geometry::AbstractVolume* getVolumeRw();

    const Ra::Core::Utils::Index* roIndexRead() const;

  private:
    Ra::Core::Utils::Index m_volumeIndex{};
    std::string m_contentName{};
    // directly hold a reference to the Abstract to simplify accesses in handlers
    std::shared_ptr<Engine::VolumeObject> _displayVolume{nullptr};
};

} // namespace Engine
} // namespace Ra

#endif // RADIUMENGINE_GEOMETRY_COMPONENT_HPP
