#ifndef RADIUMENGINE_ENGINE_HPP
#define RADIUMENGINE_ENGINE_HPP

#include <mutex>
#include <map>
#include <string>
#include <memory>
#include <vector>

#include <Core/CoreMacros.hpp>

#include <Engine/Renderer/Drawable/DrawableManager.hpp>
#include <Engine/Entity/ComponentManager.hpp>
#include <Engine/Entity/EntityManager.hpp>

namespace Ra { namespace Core   { struct MouseEvent;        } }
namespace Ra { namespace Core   { struct KeyEvent;          } }
namespace Ra { namespace Engine { class System;             } }
namespace Ra { namespace Engine { class Entity;             } }
namespace Ra { namespace Engine { class Component;          } }

// FIXME(Charly): Should the engine know about the renderer ?
namespace Ra { namespace Engine { class Renderer;           } }

namespace Ra { namespace Engine {

class RadiumEngine
{
public:
    RadiumEngine();
    ~RadiumEngine() {}

    void start();

    void initialize();
    void setupScene();
    void cleanup();

    void quit();

    // FIXME(Charly): Should the engine know the renderer ?
    Renderer* getRenderer() const { return m_renderer; }

    System* getSystem(const std::string& system) const;

    Entity* createEntity();
    void addComponent(Component* component, Entity* entity, const std::string& system);

    std::vector<Entity*> getEntities() const;

    bool loadFile(const std::string & file);

    bool handleMouseEvent(const Core::MouseEvent& event);
    bool handleKeyEvent(const Core::KeyEvent& event);

	/// Manager getters
	DrawableManager*  getDrawableManager()  const;
	EntityManager*    getEntityManager()    const;
	ComponentManager* getComponentManager() const;

private:
    void run();
    bool quitRequested();

private:
    bool m_quit;
    mutable std::mutex m_quitMutex;
    mutable std::mutex m_managersMutex;

    std::map<std::string, std::shared_ptr<System>> m_systems;
	
	std::unique_ptr<DrawableManager>  m_drawableManager;
    std::unique_ptr<EntityManager>    m_entityManager;
	std::unique_ptr<ComponentManager> m_componentManager;

	// FIXME(Charly): Pointer to the renderer needed ? 
    Renderer* m_renderer;
};

} // namespace Engine
} // namespace Ra

#endif // RADIUMENGINE_ENGINE_HPP
