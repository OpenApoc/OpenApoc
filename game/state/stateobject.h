#pragma once

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"
#include <exception>

namespace OpenApoc
{
class GameState;

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
		if (obj)
			return;
		if (id == "")
			return;
		if (!state)
		{
			LogError("Trying to resolve \"%s\" with invalid state", id.c_str());

			throw std::runtime_error(
			    UString::format("Trying to resolve \"%s\" with invalid state", id.c_str()).str());
		}
		if (obj)
		{
			LogError("Already resolved %s with ID \"%s\"", T::getTypeName().c_str(), id.c_str());
			return;
		}
		auto &prefix = T::getPrefix();
		auto idPrefix = id.substr(0, prefix.length());
		if (prefix != idPrefix)
		{
			LogError("%s object has invalid prefix - expected \"%s\" ID \"%s\"",
			         T::getTypeName().c_str(), T::getPrefix().c_str(), id.c_str());
			throw std::runtime_error(
			    UString::format("%s object has invalid prefix - expected \"%s\" ID \"%s\"",
			                    T::getTypeName().c_str(), T::getPrefix().c_str(), id.c_str())
			        .str());
		}
		obj = T::get(*state, id);
		if (!obj)
		{
			LogError("No %s object matching ID \"%s\" found", T::getTypeName().c_str(), id.c_str());
			throw std::runtime_error(UString::format("No %s object matching ID \"%s\"",
			                                         T::getTypeName().c_str(), id.c_str())
			                             .str());
		}
	}

  public:
	UString id;
	StateRef() : state(nullptr){};
	StateRef(const GameState *state) : state(state) {}
	StateRef(const GameState *state, const UString &id) : state(state), id(id) {}
	StateRef(const StateRef<T> &other) : obj(other.obj), state(other.state), id(other.id) {}

	StateRef(const GameState *state, sp<T> ptr) : obj(ptr), state(state)
	{
		if (obj)
			id = T::getId(*state, obj);
	}

	T &operator*()
	{
		resolve();
		return *obj;
	}
	const T &operator*() const
	{
		resolve();
		return *obj;
	}
	T *operator->()
	{
		resolve();
		return obj.get();
	}
	const T *operator->() const
	{
		resolve();
		return obj.get();
	}
	operator sp<T>()
	{
		resolve();
		return obj;
	}
	operator const sp<T>() const
	{
		resolve();
		return obj;
	}
	explicit operator const bool() const
	{
		resolve();
		return !!obj;
	}
	bool operator==(const StateRef<T> other) const
	{
		resolve();
		other.resolve();
		return obj == other.obj;
	}
	bool operator!=(const StateRef<T> other) const
	{
		resolve();
		other.resolve();
		return obj != other.obj;
	}
	bool operator==(const sp<T> other) const
	{
		resolve();
		return obj == other;
	}
	bool operator!=(const sp<T> other) const
	{
		resolve();
		return obj != other;
	}
	bool operator==(const T *other) const
	{
		resolve();
		return obj.get() == other;
	}
	bool operator!=(const T *other) const
	{
		resolve();
		return obj.get() != other;
	}
	StateRef<T> &operator=(const StateRef<T> &other)
	{
		state = other.state;
		obj = other.obj;
		id = other.id;
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
		resolve();
		return obj;
	}
	const bool operator<(const StateRef<T> other) const { return this->id < other.id; }
	void clear()
	{
		this->obj = nullptr;
		this->id = "";
	}
};

} // namespace OpenApoc
