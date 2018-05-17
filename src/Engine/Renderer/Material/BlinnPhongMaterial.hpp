#ifndef RADIUMENGINE_BLINNPHONGMATERIAL_HPP
#define RADIUMENGINE_BLINNPHONGMATERIAL_HPP

#include <Engine/RaEngine.hpp>

#include <map>
#include <string>

#include <Engine/Renderer/Material/Material.hpp>

// just needed for struct TextureData
#include <Engine/Renderer/Texture/TextureManager.hpp>

namespace Ra {
namespace Engine {
class Texture;
class ShaderProgram;

class RA_ENGINE_API BlinnPhongMaterial final : public Material {
  public:
    enum class TextureType { TEX_DIFFUSE, TEX_SPECULAR, TEX_NORMAL, TEX_SHININESS, TEX_ALPHA };

  public:
    RA_CORE_ALIGNED_NEW

    explicit BlinnPhongMaterial( const std::string& name );

    ~BlinnPhongMaterial();

    const std::string getShaderInclude() const override;

    void updateGL() override;
    void bind( const ShaderProgram* shader ) override;
    bool isTransparent() const override;

    inline void addTexture( const TextureType& type, Texture* texture );
    inline TextureData& addTexture( const TextureType& type, const std::string& texture );
    inline TextureData& addTexture( const TextureType& type, const TextureData& texture );
    inline Texture* getTexture( const TextureType& type ) const;

  public:
    Core::Math::Color m_kd;
    Core::Math::Color m_ks;
    Scalar m_ns;
    Scalar m_alpha;

  private:
    std::map<TextureType, Texture*> m_textures;
    std::map<TextureType, TextureData> m_pendingTextures;
};

} // namespace Engine
} // namespace Ra

#include <Engine/Renderer/Material/BlinnPhongMaterial.inl>
#endif // RADIUMENGINE_BLINNPHONGMATERIAL_HPP
