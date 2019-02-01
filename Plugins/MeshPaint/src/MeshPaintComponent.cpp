#include <MeshPaintComponent.hpp>

#include <iostream>

#include <Core/Utils/Log.hpp>

#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Tasks/TaskQueue.hpp>

#include <Engine/Entity/Entity.hpp>
#include <Engine/Renderer/Material/Material.hpp>
#include <Engine/Renderer/Mesh/Mesh.hpp>
#include <Engine/Renderer/RenderObject/RenderObject.hpp>
#include <Engine/Renderer/RenderObject/RenderObjectManager.hpp>
#include <Engine/Renderer/RenderTechnique/RenderTechnique.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderConfigFactory.hpp>

using namespace Ra::Core::Utils; // log

namespace MeshPaintPlugin {

static const std::string colAttribName =
    Ra::Engine::Mesh::getAttribName( Ra::Engine::Mesh::VERTEX_COLOR );

MeshPaintComponent::MeshPaintComponent( const std::string& name, Ra::Engine::Entity* entity ) :
    Ra::Engine::Component( name, entity ) {}

MeshPaintComponent::~MeshPaintComponent() {}

void MeshPaintComponent::addTasks( Ra::Core::TaskQueue* /*taskQueue*/,
                                   const Ra::Engine::FrameInfo& /*info*/ ) {}

void MeshPaintComponent::setDataId( const std::string& id ) {
    m_dataId = id;
}

void MeshPaintComponent::initialize() {
    using Ra::Engine::Mesh;
    auto compMess = Ra::Engine::ComponentMessenger::getInstance();
    // Look for the data we need
    bool geometryData = compMess->canGet<Ra::Core::Geometry::TriangleMesh>( getEntity(), m_dataId );

    if ( !geometryData )
    {
        LOG( logWARNING )
            << "Cannot initialize MeshPaint Component: no TriangleMesh in geometry data."
            << std::endl;
        return;
    }

    m_renderObjectReader =
        compMess->getterCallback<Ra::Core::Utils::Index>( getEntity(), m_dataId );

    auto ro = getRoMgr()->getRenderObject( *m_renderObjectReader() );
    m_baseConfig = ro->getRenderTechnique()->getConfiguration();

    Mesh* mesh = dynamic_cast<Mesh*>( ro->getMesh().get() );
    CORE_ASSERT( mesh != nullptr, "Cannot access Mesh from Ro containing a TriangleMesh" );

    // Could also be accessed using
    // auto triangleMesh = compMess->get<Ra::Core::Geometry::TriangleMesh>( getEntity(), m_dataId );
    // however here we skip the search in the component map
    Ra::Core::Geometry::TriangleMesh& triangleMesh = mesh->getTriangleMesh();

    // In any, save colors from the CPU object, otherwise set default color and notify GPU object
    m_currentColorAttribHdl = triangleMesh.getAttribHandle<Ra::Core::Vector4>( colAttribName );
    if ( m_currentColorAttribHdl.idx().isInvalid() )
    {
        m_baseColors.clear();
        m_currentColorAttribHdl = triangleMesh.addAttrib<Ra::Core::Vector4>( colAttribName );
        triangleMesh.getAttrib( m_currentColorAttribHdl )
            .data()
            .resize( triangleMesh.vertices().size(), Ra::Core::Utils::Color::Skin() );
        mesh->setDirty( Mesh::VERTEX_COLOR, true );
    } else
    {
        m_baseColors = triangleMesh.getAttrib( m_currentColorAttribHdl ).data(); // copy
    }
}

void MeshPaintComponent::startPaint( bool on ) {
    auto ro = getRoMgr()->getRenderObject( *m_renderObjectReader() );
    if ( on )
    {
        ro->getRenderTechnique()->setConfiguration(
            Ra::Engine::ShaderConfigurationFactory::getConfiguration( "Plain" ) );
    } else
    {
        Ra::Engine::Mesh* mesh = dynamic_cast<Ra::Engine::Mesh*>( ro->getMesh().get() );
        CORE_ASSERT( mesh != nullptr, "Cannot access Mesh from Ro containing a TriangleMesh" );
        ON_DEBUG( auto colAttrib =
                      mesh->getTriangleMesh().getAttribHandle<Ra::Core::Vector4>( colAttribName ) );
        CORE_ASSERT( colAttrib == m_currentColorAttribHdl, "Inconsistent AttribHandle used" );
        // Could also be accessed using
        // auto triangleMesh = compMess->get<Ra::Core::Geometry::TriangleMesh>( getEntity(),
        // m_dataId ); however here we skip the search in the component map
        Ra::Core::Geometry::TriangleMesh& triangleMesh = mesh->getTriangleMesh();
        triangleMesh.getAttrib( m_currentColorAttribHdl ).data() = m_baseColors;

        ro->getRenderTechnique()->setConfiguration( m_baseConfig );
    }
}

void MeshPaintComponent::paintMesh( const Ra::Engine::Renderer::PickingResult& picking,
                                    const Ra::Core::Utils::Color& color ) {
    using Ra::Engine::Mesh;
    // check it's for us
    if ( *m_renderObjectReader() != picking.m_roIdx || picking.m_mode == Ra::Engine::Renderer::RO )
    {
        return;
    }

    auto ro = getRoMgr()->getRenderObject( *m_renderObjectReader() );
    auto pickingRenderMode = ro->getMesh()->pickingRenderMode();

    if ( pickingRenderMode == Ra::Engine::Displayable::NO_PICKING )
    {
        return;
    }

    if ( pickingRenderMode == Ra::Engine::Displayable::PKM_POINTS &&
         ( picking.m_mode != Ra::Engine::Renderer::VERTEX &&
           picking.m_mode != Ra::Engine::Renderer::C_VERTEX ) )
    {
        return;
    }

    Mesh* mesh = dynamic_cast<Mesh*>( ro->getMesh().get() );
    CORE_ASSERT( mesh != nullptr, "We should be have a Mesh storing a TriangleMesh here" );

    // Could also be accessed using
    // auto triangleMesh = compMess->get<Ra::Core::Geometry::TriangleMesh>( getEntity(), m_dataId );
    // however here we skip the search in the component map
    const auto& T = mesh->getTriangleMesh().m_triangles;
    ON_DEBUG( auto colAttrib =
                  mesh->getTriangleMesh().getAttribHandle<Ra::Core::Vector4>( colAttribName ) );
    CORE_ASSERT( colAttrib == m_currentColorAttribHdl, "Inconsistent AttribHandle used" );
    Ra::Core::VectorArray<Ra::Core::Vector4>& colorContainer =
        mesh->getTriangleMesh().getAttrib( m_currentColorAttribHdl ).data();

    switch ( picking.m_mode )
    {
    case Ra::Engine::Renderer::VERTEX:
        [[fallthrough]];
    case Ra::Engine::Renderer::C_VERTEX:
    {
        if ( pickingRenderMode == Ra::Engine::Displayable::PKM_POINTS )
        {
            for ( auto t : picking.m_elementIdx )
            {
                colorContainer[size_t( t )] = color;
            }
        } else
        {
            for ( size_t t = 0; t < picking.m_elementIdx.size(); ++t )
            {
                size_t v = T[size_t( picking.m_elementIdx[t] )]( picking.m_vertexIdx[t] );
                colorContainer[v] = color;
            }
        }
        mesh->setDirty( Mesh::VERTEX_COLOR );
        break;
    }
    case Ra::Engine::Renderer::EDGE:
        [[fallthrough]];
    case Ra::Engine::Renderer::C_EDGE:
    {
        for ( size_t t = 0; t < picking.m_elementIdx.size(); ++t )
        {
            const auto& el = T[size_t( picking.m_elementIdx[t] )];
            size_t v1 = el( ( picking.m_edgeIdx[t] + 1 ) % 3 );
            size_t v2 = el( ( picking.m_edgeIdx[t] + 2 ) % 3 );
            colorContainer[v1] = colorContainer[v2] = color;
        }
        mesh->setDirty( Mesh::VERTEX_COLOR );
        break;
    }
    case Ra::Engine::Renderer::TRIANGLE:
        [[fallthrough]];
    case Ra::Engine::Renderer::C_TRIANGLE:
    {
        for ( size_t t = 0; t < picking.m_elementIdx.size(); ++t )
        {
            const auto& el = T[size_t( picking.m_elementIdx[t] )];
            size_t v1 = el( 0 );
            size_t v2 = el( 1 );
            size_t v3 = el( 2 );
            colorContainer[v1] = colorContainer[v2] = colorContainer[v3] = color;
        }
        mesh->setDirty( Mesh::VERTEX_COLOR );
        break;
    }
    default:
        break;
    }
}

} // namespace MeshPaintPlugin
