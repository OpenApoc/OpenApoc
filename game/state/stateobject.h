#pragma once

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/strings_format.h"
#include <exception>
#include <map>

namespace OpenApoc
{
class GameState;

uint64_t getNextObjectID(GameState &state, const UString &objectPrefix);

template <typename T> class StateRefMap : public std::map<UString, sp<T>>
{
  public:
	~StateRefMap()
	{
		for (auto &obj : *this)
		{
			obj.second->destroy();
		}
	}
};

template <typename T> class StateObject
{
  public:
	StateObject() = default;
	virtual ~StateObject() = default;

	static sp<T> get(const GameState &state, const UString &id);
	static const UString &getId(const GameState &state, const sp<T> ptr);
	static const UString &getPrefix();
	static const UString &getTypeName();
	static UString generateObjectID(GameState &state)
	{
		auto id = getNextObjectID(state, getPrefix());
		return getPrefix() + Strings::fromU64(id);
	}

	virtual void destroy(){};
	// StateObjects are not copy-able
	StateObject(const StateObject &) = delete;
	// Move is fine
	StateObject(StateObject &&) = default;
};

template <typename T> class StateRef
{
	// This would be nice, but means you can't have a member of a class containing a stateref to the
	// same class type
	//	static_assert(std::is_base_of<StateObject<T>, T>::value,
	//	              "StateRef must only reference StateObject subclasses");

  private:
	mutable sp<T> obj;
	const GameState *state;

	void resolve() const
	{
		if (id.empty())
			return;
		auto &prefix = T::getPrefix();
		auto idPrefix = id.substr(0, prefix.length());
		if (prefix != idPrefix)
		{
			LogWarning("%s object has invalid prefix - expected \"%s\" ID \"%s\"", T::getTypeName(),
			           T::getPrefix(), id);
		}
		obj = T::get(*state, id);
		if (!obj)
		{
			LogError("No %s object matching ID \"%s\" found", T::getTypeName(), id);
		}
	}

  public:
	UString id;
	StateRef() : state(nullptr){};
	StateRef(const GameState *state) : state(state) {}
	StateRef(const GameState *state, const UString &id) : state(state), id(id) {}
	StateRef(const StateRef<T> &other) = default;

	StateRef(const GameState *state, sp<T> ptr) : obj(ptr), state(state)
	{
		if (obj)
			id = T::getId(*state, obj);
	}

	T &operator*()
	{
		if (!obj)
			resolve();
		return *obj;
	}
	const T &operator*() const
	{
		if (!obj)
			resolve();
		return *obj;
	}
	T *operator->()
	{
		if (!obj)
			resolve();
		return obj.get();
	}
	const T *operator->() const
	{
		if (!obj)
			resolve();
		return obj.get();
	}
	operator sp<T>()
	{
		if (!obj)
			resolve();
		return obj;
	}
	operator const sp<T>() const
	{
		if (!obj)
			resolve();
		return obj;
	}
	explicit operator bool() const
	{
		if (!obj)
			resolve();
		return !!obj;
	}
	bool operator==(const StateRef<T> &other) const
	{
		if (this->id != other.id)
		{
			return false;
		}
		return true;
	}
	bool operator!=(const StateRef<T> &other) const { return !(*this == other); }
	bool operator==(const sp<T> &other) const
	{
		if (!obj)
			resolve();
		return obj == other;
	}
	bool operator!=(const sp<T> &other) const
	{
		if (!obj)
			resolve();
		return obj != other;
	}
	bool operator==(const T *other) const
	{
		if (!obj)
			resolve();
		return obj.get() == other;
	}
	bool operator!=(const T *other) const
	{
		if (!obj)
			resolve();
		return obj.get() != other;
	}
	StateRef<T> &operator=(const StateRef<T> &other) = default;
	// Explicitly handle "object = nullptr", as otherwise gcc doesn't know which overload to use
	StateRef<T> &operator=(std::nullptr_t)
	{
		this->clear();
		return *this;
	}
	StateRef<T> &operator=(const UString newId)
	{
		obj = nullptr;
		id = newId;
		return *this;
	}
	sp<T> getSp() const
	{
		if (!obj)
			resolve();
		return obj;
	}
	const T *get() const
	{
		if (!obj)
			resolve();
		return obj.get();
	}
	bool operator<(const StateRef<T> &other) const { return this->id < other.id; }
	void clear()
	{
		this->obj = nullptr;
		this->id = "";
	}
};

} // namespace OpenApoc
