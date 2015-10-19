#include <Plugins/FancyMesh/FancyMeshComponent.hpp>

#include <Core/String/StringUtils.hpp>
#include <Engine/Renderer/RenderObject/RenderObjectManager.hpp>
#include <Engine/Renderer/Mesh/Mesh.hpp>
#include <Engine/Renderer/RenderTechnique/RenderTechnique.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderConfiguration.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderProgramManager.hpp>

namespace FancyMeshPlugin
{
    FancyMeshComponent::FancyMeshComponent( const std::string& name )
        : Ra::Engine::Component( name )
    {
    }

    FancyMeshComponent::~FancyMeshComponent()
    {
        // TODO(Charly): Should we ask the RO manager to delete our render object ?
    }

    void FancyMeshComponent::initialize()
    {
    }

    void FancyMeshComponent::addMeshRenderObject( const Ra::Core::TriangleMesh& mesh, const std::string& name )
    {
        Ra::Engine::RenderTechnique* technique = new Ra::Engine::RenderTechnique;
        technique->material = new Ra::Engine::Material( "Default" );
        technique->shaderConfig = Ra::Engine::ShaderConfiguration( "Default", "../Shaders" );

        addMeshRenderObject(mesh, name, technique);
    }

    void FancyMeshComponent::addMeshRenderObject( const Ra::Core::TriangleMesh& mesh,
                                                  const std::string& name,
                                                  Ra::Engine::RenderTechnique* technique )
    {
        Ra::Engine::RenderObject* renderObject = new Ra::Engine::RenderObject( name, this );
        renderObject->setVisible( true );

        renderObject->setRenderTechnique( technique );

        if ( technique->material->getMaterialType() == Ra::Engine::Material::MaterialType::MAT_TRANSPARENT )
        {
            renderObject->setType( Ra::Engine::RenderObject::Type::RO_TRANSPARENT );
        }

        Ra::Engine::Mesh* displayMesh = new Ra::Engine::Mesh( name );
        std::vector<uint> indices;
        indices.reserve( mesh.m_triangles.size() * 3 );
        for ( const auto& i : mesh.m_triangles )
        {
            indices.push_back( i.x() );
            indices.push_back( i.y() );
            indices.push_back( i.z() );
        }

        displayMesh->loadGeometry( mesh.m_vertices, indices );
        displayMesh->addData( Ra::Engine::Mesh::VERTEX_NORMAL, mesh.m_normals );

        renderObject->setMesh( displayMesh );

        addRenderObject( renderObject );
    }


    void FancyMeshComponent::handleMeshLoading( const FancyComponentData& data )
    {
        CORE_ASSERT( data.meshes.size() == 1, "One mesh per component / object." );
        // FIXME(Charly): Change data meshes array to just one mesh

        Ra::Engine::RenderObject* renderObject = new Ra::Engine::RenderObject( data.name, this );
        renderObject->setVisible( true );

        for ( uint i = 0; i < data.meshes.size(); ++i )
        {
            FancyMeshData meshData = data.meshes[i];

            std::stringstream ss;
            ss << data.name << "_mesh_" << i;
            std::string meshName = ss.str();

            Ra::Engine::Mesh* mesh = new Ra::Engine::Mesh( meshName );

            mesh->loadGeometry( meshData.positions, meshData.indices );

            mesh->addData( Ra::Engine::Mesh::VERTEX_NORMAL, meshData.normals );
            mesh->addData( Ra::Engine::Mesh::VERTEX_TANGENT, meshData.tangents );
            mesh->addData( Ra::Engine::Mesh::VERTEX_BITANGENT, meshData.bitangents );
            mesh->addData( Ra::Engine::Mesh::VERTEX_TEXCOORD, meshData.texcoords );
            mesh->addData( Ra::Engine::Mesh::VERTEX_COLOR, meshData.colors );
            mesh->addData( Ra::Engine::Mesh::VERTEX_WEIGHTS, meshData.weights );

            renderObject->setMesh( mesh );
        }

        renderObject->setRenderTechnique( data.renderTechnique );

        m_meshIndex = addRenderObject(renderObject);
    }
    
    void FancyMeshComponent::setLoadingInfo(MeshLoadingInfo info)
    {
        m_loadingInfo = info;
    }
    
    MeshLoadingInfo FancyMeshComponent::getLoadingInfo() const
    {
        return m_loadingInfo;
    }
    
    Ra::Core::Index FancyMeshComponent::getMeshIndex() const
    {
        return m_meshIndex;
    }
    
} // namespace FancyMeshPlugin
