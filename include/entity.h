#pragma once

#include <functional>

#include <entt/entity/registry.hpp>

#include "components.h"

class Scene;

class Entity
{
	friend Scene;
public:
	Entity(uint32_t id, Scene* scene) : m_EntityID((entt::entity)id), m_ScenePtr(scene) {};
	Entity(entt::entity id, Scene* scene) : m_EntityID((entt::entity)id), m_ScenePtr(scene) {};
	Entity(const Entity& o) = default;

	template<typename... Components>
	bool HasComponents()
	{
		// TODO: Maybe check if m_EntityID is valid
		return m_ScenePtr->m_EntityRegistry.all_of<Components...>(m_EntityID);
	}

	template<typename Component>
	Component& GetComponent()
	{
		entt::registry& reg = m_ScenePtr->m_EntityRegistry;
		if (HasComponents<Component>())
		{
			return reg.get<Component>(m_EntityID);
		}
		else
		{
			std::cerr << "[Error] Trying to access non existing component '" << typeid(Component).name() << "' for entity '" << reg.get<NameComponent>(m_EntityID).name << "'" << std::endl;
			std::cout << "[Info] Creating new component of type: '" << typeid(Component).name() << "'" << std::endl;
			return AddComponent<Component>();
		}
	}

	template<typename Component>
	Component& AddComponent(const Component& c) { 
		entt::registry& reg = m_ScenePtr->m_EntityRegistry;
		if (HasComponents<Component>())
			std::cerr << "Warning: trying to add multiple components of the same type to entity '" << reg.get<NameComponent>(m_EntityID).name << "'" << std::endl;
		return reg.get_or_emplace<Component>(m_EntityID, c);
	}

	template<typename Component>
	Component& AddComponent() {
		return AddComponent<Component>(Component());
	}

	uint32_t GetID() { return (uint32_t)m_EntityID;  }
	operator uint32_t() { return (uint32_t) m_EntityID; }

	static Entity Invalid() { return { -1u, nullptr }; }

private:
	operator entt::entity() { return (entt::entity)m_EntityID; }

private:
	Entity() = delete;

	entt::entity m_EntityID;
	Scene* m_ScenePtr;
};