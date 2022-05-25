#pragma once

#include <functional>

#include <entt/entity/registry.hpp>

#include "components.h"

class Entity
{
public:
	static Entity Create(const char* name)
	{
		Entity e = Entity(s_Registry.create());
		e.AddComponent<NameComponent>(name);
		e.AddComponent<TransformComponent>();
		return e;
	}

	static bool Exists(uint32_t id)
	{
		return s_Registry.valid((entt::entity)id);
	}

	static Entity Get(uint32_t id)
	{
		return Entity(id);
	}

	static Entity Entity::Invalid()
	{
		return Entity(-1);
	}

	inline uint32_t GetID() { return (uint32_t) m_EntityID; }

	template<typename T>
	inline decltype(auto) AddComponent() 
	{
		return s_Registry.emplace_or_replace<T>(m_EntityID, T());
	}
	template<typename T>
	inline decltype(auto) AddComponent(T component) 
	{
		return s_Registry.emplace_or_replace<T>(m_EntityID, component);
	}
	template<typename T>
	inline decltype(auto) GetComponent() 
	{
		if (!s_Registry.all_of<T>(m_EntityID))
		{
			std::string& name = s_Registry.get<NameComponent>(m_EntityID).name;
			std::cerr << "Error: entity '" << name << "' does not have the requested component." << std::endl;
		}
		return s_Registry.get<T>(m_EntityID);
	}

	template<typename... Ts>
	static void ForEachWith(std::function<void(Entity)> fn)
	{
		auto view = s_Registry.view<Ts...>();
		for (auto entity : view)
		{
			if (!s_Registry.all_of<Ts...>(entity))
			{
				std::string& name = s_Registry.get<NameComponent>(entity).name;
				std::cerr << "Error: entity '" << name << "' does not have all of the requested components." << std::endl;
				return;
			}

			fn(entity);
		}
	};

private:
	Entity() = delete;
	Entity(uint32_t id) : m_EntityID((entt::entity)id) {};
	Entity(entt::entity id) : m_EntityID(id) {};

	entt::entity m_EntityID;

	static entt::registry s_Registry;
};