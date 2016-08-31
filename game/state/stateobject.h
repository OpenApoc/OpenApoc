#pragma once

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"
#include <exception>
#include <map>

namespace OpenApoc
{
class GameState;

template <typename T> class StateRefMap : public std::map<UString, sp<T>>
{
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
#ifndef NDEBUG
		auto &prefix = T::getPrefix();
		auto idPrefix = id.substr(0, prefix.length());
		if (prefix != idPrefix)
		{
			LogError("%s object has invalid prefix - expected \"%s\" ID \"%s\"",
			         T::getTypeName().cStr(), T::getPrefix().cStr(), id.cStr());
			throw std::runtime_error(
			    UString::format("%s object has invalid prefix - expected \"%s\" ID \"%s\"",
			                    T::getTypeName(), T::getPrefix(), id)
			        .str());
		}
#endif
		obj = T::get(*state, id);
#ifndef NDEBUG
		if (!obj)
		{
			LogError("No %s object matching ID \"%s\" found", T::getTypeName().cStr(), id.cStr());
			throw std::runtime_error(
			    UString::format("No %s object matching ID \"%s\"", T::getTypeName(), id).str());
		}
#endif
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
	bool operator==(const StateRef<T> &other) const { return (this->id == other.id); }
	bool operator!=(const StateRef<T> &other) const { return (this->id != other.id); }
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
	bool operator<(const StateRef<T> &other) const { return this->id < other.id; }
	void clear()
	{
		this->obj = nullptr;
		this->id = "";
	}
};

} // namespace OpenApoc
