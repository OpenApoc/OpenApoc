Openapoc Coding Style Guidelines
================================

This document specifies the guidelines for writing and formatting the c++ code that forms the core of OpenApoc.

Globally, we use 'standard' c++17. This requires reasonably modern compilers (gcc 8, MSVC 2019+ , clang 7+ have been tested). You should avoid using compiler-specific stuff where possible. Exceptions to this exist, but should be wrapped in a #ifdef:
```C++
#ifdef _MSC_VER
MSVCIsCrazySometimes
#else
// Everything else we support (gcc + clang) are pretty much extension-compatible
GCCIsntMuchBetter
#endif
```

C++11 features are heavily encouraged - patterns from 'older' c++ versions that have been superceded should be avoided.

The formatting sections of this document are enforced by the [clang-format tool](http://llvm.org/releases/8.0.0/tools/clang/docs/ClangFormat.html). Currently, version '8.0' of ``clang-format`` is to be used. The configuration file ``.clang-format`` in the root of the OpenApoc source repository should match the formatting guidelines specified below.

With this, it is highly recommended to run ``clang-format`` on all modified files before check-in. This can be run on source files with the following command:

```
clang-format path/to/file.cpp path/to/file.h
```

When ran from the root of the OpenApoc source repository it should automatically use the supplied ``.clang-format`` configuration file. The tool also supports modifying the supplied source files to match the configured format when passed the ``-i`` flag:

```
clang-format -i path/to/file.cpp path/to/file.h
```

When using the ``CMake`` build system (as used on Unix-based platforms), there is a ``format-sources`` make target that will run ``clang-format -i`` on all source files within the OpenApoc repositry tree. This provides a single command that can be run before commiting:

```
make format-sources
```

In the future, it may be possible to use ``clang-tidy``, also from the clang project, to enforce more of the non-formatting sections of this style guideline. One example of this is an identifier name checking tool.


Indent:
-------
* Tabs for indenting, spaces for alignment, indenting by 1 tab for each new scope
```C++
void function()
{
	reallyLongFunctionNameWithLotsOfArguments(argOne, argTwo,
	                                          argThree);
}
```
* Avoid going over 100 cols (at tab width of '4' spaces).
  * If you find yourself going over this it's often a hint to try to pull things out of loops/into functions
  * Don't break strings up to fit this, it looks ugly and makes things even harder to read.
* If you have to break, indent the following line by an extra tab
  * When breaking a single statement, break the line before the next operator. Avoid having an operator as the last thing on a line.
```C++
void reallyLongFunctionNameIMeanThisIsReallyBadlyNamedWhateverIDontCareTheyPayMeAnyway(int parameterOne,
	int paramTwo, char theThirdOne)
{
	if (parameterOne == yetAnotherReallyLongLineHereComeSomeWordsBlaBlaBlaAreYouStillReadingThisComeOnDont
		&& youHaveBetterThingsToDo)
	{
		doWhatever();
	}
}
```

Whitespace:
-----------
* Spaces before and after operators
```C++
	a = b;
	a && b;
	a + b;
```
* Space after if/when/else/for, space after :/; in for
```C++
	for (auto &a: b)
```
* No spaces after function name (or function-like keywords like 'sizeof'), but space after flow control keywords, space after comma for multiple args
```C++
	func(a, b);
	if (a == 0)
```
* References and pointers: & and * align to right (to variable) not type
```C++
	float *pointerToFloat;
```

Scope:
------
* Indent 1 tab for each new scope
* New scope is _always_ surrounded by {} braces
* New scope has a '{' on the /next/ line at the indent of the old scope(not the new scope)
* closing scope '}' same indent at opening '{', again on a new line
* New scope caused by:
  * Functions
```C++
void functionDefinition()
{
	newScopeHere();
}
```
  * new conditional block (if/else/when/for)
```C++
	if (x)
	{
		doWhatever();
	}
	else if (y)
	{
		doWhateverTheSecond();
	}
	else
	{
		doThatLastThing();
	}

```
  * 'switch'
  * 'case' also indents a new scope. {} are optional, based on if new stack variables are needed to handle the case.
    * Note switch should always have a default case unless over an enum class (where they may not if (and only if) every value is handled)
    * All 'case' sections should have a 'break'
```C++
	switch (a)
	{
		case A:
			doSomething();
			break;
		case B:
		{
			auto var = somethingElse();
			doSomethingElse(var);
			break;
		}
		default:
			doDefaultCase();
			break;
	}
```
  * Class/enum/struct declarations
    * note: public/private/protected are an exception to this, being aligned to 'class' keyword, not 'within' it's scope
```C++
class MyClass
{
private:
	int privateVariable;
public:
	void publicFunction();
};
```
* Exception to this is 'trivial' functions that have the definition & contents all on line line
  * 'Trivial' is defined by a single statement that fits within the 100-column limit
```C++
int Class::function() {return 0;}
```
* New scope is not caused by:
  * namespace (which should also have a comment stating the namespace name at the closing bracket)
```C++
namespace OpenApoc
{
Class MyClass
{
private:
	int x;
};
}; // namespace OpenApoc
```
  * labels
* Labels and #preprocessor directives /always/ on column 0 (start of line) no matter the scope
```C++
#if defined(LINUX)
	x = linuxFunction();
#else
	x = otherFunction();
#endif
	if (x)
	{
		goto error;
	}
error:
	return 0;
```

Case:
-----
* Namespaces should be CamelCase
```C++
namespace OpenApoc
```
* Classes and enums should be CamelCase
```C++
class MyClass
```
* 'enum class' members should be CamelCase
```C++
enum class MyEnum
{
	ValueOne,
	ValueTwo,
};
```
* class methods and member variables (public/private/protected) should be camelBack
```C++
class MyClass
{
public:
	void someFunction();
	int someVariable;
};
```
* Function parameters (public/private/protected) should be camelBack
```C++
int function(int parameterOne, char secondOne)
```
* Variables should be camelBack
  * Don't be afraid to use 'short' variable names if it's obvious
```C++
void function()
{
	int localVariable = 0;
	int x = 1;
	int y = 5;
	for (int i = 0; i < 5; i++)
```
* class/global constants should be SHOUTY_CAPS, along with _all_ macros
```C++
#define OPENAPOC_VERSION "1.0"
```
* Labels should be lower_case:
```C++
exit_loop:
	goto exit_loop;
```
* All members should be initialised in all constructors if they don't have a default constructor. You can use member initialisation in the class definition if this is clearer
```C++
class MyClass
{
	int variableMember = 0;
};
```

Types:
------
* Avoid typedef - use 'struct' keyword where necessary in c-like code
```C++
struct MyStruct
{
	int x;
};

void myStructUser(struct MyStruct s)
```
* up<> sp<> wp<> aliases are defined for std::unique_ptr<>, std::shared_ptr<>, std::weak_ptr<> in "library/sp.h". Use them instead of the verbose versions.
* Use anonymous namespaces for 'file-local' stuff (instead of static, as you can wrap classes in it too)
```C++
namespace
{
void localFunction()
}; // anonymous namespace
```
* We provide a "UString" class. This should be used for _all_ strings, as it provides platform-local non-ascii charset handling
  * All "char*"/std::string params are assumed to be in utf8 format.


Templates:
----------
* If templates help, go ahead, don't avoid them
* prefer 'typename' to 'class'
* template types should be CamelCase
```C++
template <typename LocalType> function(LocalType param)
```
* 'short' typenames are OK if it's obvious what's going on
```C++
template <typename T> Class<T>::function()
```

Class declarations:
-------------------
* member functions camelCase()
* 'public:' 'private:' 'protected:' are indented to the 'class' keyword, everything within them indented to class scope.
  * Always use 'private', even if that's the default
```C++
class MyClass
{
private:
	int localVariable;
public:
	void publicFunction();
};
```
* 'virtual' keyword only used for base class, 'override' used for derived
  * All classes with a virtual (or overrided) function _must_ specify a virtual destructor
* Inheritence should be on the same line as the 'class' keyword (until you get to the column limit and break)
```C++
class BaseClass
{
public:
	virtual ~BaseClass();
	virtual void someFunction();
};

class SubClass : public BaseClass
{
public:
	void someFunction() override;
};
```
  * Never use both 'virtual' and 'override
* Don't define an empty {} body in the header for constructors/destructors etc. - use '= default' instead
```C++
class MyBaseClass
{
public:
	virtual ~MyBaseClass() = default;
};
```
* define pure virtual "virtual void func() = 0;" for 'interface' classes
```C++
class MyInterface
{
public:
	virtual void functionBaseClassesMustOverride() = 0;
};
```
  * No need for 'pure' interface classes, they can have code that all subclasses will use!
* For trivial initial values prefer initialisers in the class declaration (It's easier to see what's set and cleans up constructor definitions)
```C++
class MyClass
{
public:
	Type initialisedMember = 0;
};
```
* In constructors prefer initialisation of members with  initialiser list over assignment
  * Good:
```C++
MyClass::MyClass(Type value) : member(value)
{
	doWhatever();
}
```
  * Bad:
```C++
MyClass::MyClass(Type value)
{
	member = value;
	doWhatever();
}
```
* Initialisers should be in order of declaration in the class
  * For example with the class:
```C++
class MyClass
{
public:
	Type memberA;
	Type memberB;

	MyClass(Type valueA, Type valueB);
};
```
  * Good:
```C++
MyClass::MyClass(Type valueA, Type valueB) : memberA(valueA), memberB(valueB) {}
```
  * Bad:
```C++
MyClass::MyClass(Type valueA, Type valueB) : memberB(valueB), memberA(valueA) {}
```
* Use 'struct' for 'data-only' types
  * Structs should _never_ have public/private/protected declarations, if there's anything non-public you shouldn't use a struct.
  * Likely only going to be used within data reading/writing to files
    * Because of this you're probably going to need to use 'sized' typed (see <cstdint> header)
```C++
struct DataFileSection
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
};
```
  * If using a struct to read in data, use a static_assert to ensure correct sizing:
```C++
static_assert(sizeof(DataFileSection) == 12, "DataFileSection not expected 12 bytes");
```
* If ownership of a member is tied to the class, don't use a pointer and new/delete in constructor/destructor. Just use the type and initialise it correctly in the init list before the constructor.
  * If the above is not possible (e.g. complex init requirements, 'may be invalid and null' use a up<>
* If we /know/ a member reference owned by another object will be live for the duration of the class, use a &reference member
* Otherwise use a sp<>/up<> depending if it should take a reference and if having a 'null' object makes sense.

Functions:
----------
* Const functions where possible (IE not modifying any members)
```C++
class MyClass
{
public:
	int dataAccessor() const;
};
```
* Const params where logically not to be modified
```C++
void function(const Type& readOnlyParam)
```
* Const returns where the caller should never modify
```C++
const Type& functionWhereYouCantModifyMyReturnThanks()
```

General code:
-------------
* Where possible use auto
```C++
	auto varableName = function();
```
  * Note where auto& may be better to avoid a copy
* sp<> up<> wp<> smart pointers
  * make_shared() instead of new
```C++
	auto var = std::make_shared<Type>(args);
```
  * make_unique() would be nice, but a c++14 feature (may restrict compiler compatibility?), so you have to wrap
    * Use std::move to transfer up<> ownership
```C++
	up<Type> var{new Type{args}};
	functionThatTakesOwnershipOfParam(std::move(var));
```
* Never use a 'naked' new - they should always be packaged immediately in a smart pointer
* Use emplace() in STL containers where possible over insert()
  * Unless you explicitly want to copy an object in
* Use foreach loops where possible ( "for (auto &element : container)")
```C++
	for (auto &element : container)
	{
		whatever(element);
	}
```
  * Exception may be a 'safe' iterator when possibly removing elements during loop, then use iterator and keep copy locally
* Where possible use 'enum class'
* Naming variables - don't be afraid of using 'short' names ('i') if it's use is obvious
* While 'descriptive' names are nice, shorter code is also nice. Don't repeat context
  * 'x' is fine is we already know we're doing something in coordinate space, no need to name it theXCoordinateOfTheCityMapInTiles
* Reading code is important - try to make it flow
  * avoid 'yoda conditionals' (1 == var) don't help, modern compilers catch a =/== typo easily
  * if post increment (x++) flows better use that, don't try to 'optimise' away the copy - the compiler will do that for you
* The compiler is more clever than you could ever possibly hope to be. Write things to be clear and obvious. Only /after/ it's proven to be a problem to you even look at optimisation (then _always have numbers_)
* don't use 'c' casts ("(int)x") - that does different things on if the object type has a defined conversion or not - use static_cast<>/reinterpret_case<> instead
* prefer {} constructor calls where possible
```C++
	MyClass classInstance{argumentOne, argTwo};
```
  * Requires you to avoid implicit conversions - this is good!
* STL initialiser lists are good
```C++
	std::vector<int> someInts = {1, 2, 3};
```
* static_assert() any assumptions WRT alignment/packing (when reading structs from files, for example) - or any template restrictions (e.g. if this is only valid on unsigned types, check it!)
* <limits> is preferred to 'c' MAX_INT/whatever
```C++
	auto maxInt = std::numeric_limits<int>::max();
```
* RAII where possible
* Early-return is cool, go ahead
```C++
	if (dontHaveToDoAnything)
	{
		return;
	}
	doLotsOfBoringStuff();
```
* goto: is useful in some specific cases (e.g. breaking out of nested loops) - but only use it where another keyword won't do what you want
  * Note limitations WRT goto: over stack initialisers - IE you can't do it :)
```C++

	for (auto &x : containerX)
	{
		for (auto &y : x.containerY)
		{
			if (weShouldStopAt(y))
			{
				goto end;
			}
		}
	}
end:
	return;
```

Logging:
--------
* LogInfo/LogWarning/LogError take 'printf-style' format string. Make sure everything is the correct type, %d for ints, %u for unsigned, %s for "utf8" const char* "strings" or UString objects directly
* LogInfo is cheap - use it everywhere interesting
* LogWarning should be something that has gone wrong, but recoverable.
* LogError is for fatal errors.

Comments:
* either // or /* */ is fine - prefer // for single line
* If doing multi line /\*-style comments have an aligned '\*' with at the beginning of each subseqent line:
```C++
/* first line
 * second line
 * last line */
```
* Don't comment for the sake of it
  * Try to make the code clearer first if a comment is 'required' to make something obvious
  * Function/variable names are useful here - if reading it aloud describes what your comment was going to say that's perfect
  * //TODO: //FIXME: when leaving known breakage

Includes:
---------
* "local.h" files first
* then <system.h> includes
* Within each of the 2 blocks try to keep them alphabetically sorted (some exceptions may happen, if there's a system dependency not managed by the system header itself)
* local headers always relative to the root of OpenApoc - even if in the same directory
  * e.g. "framework/event.h" not "../event.h" or "event.h"

Headers:
--------
* prefer "#pragma once" to "#ifndef __HEADER_NAME" include guards
* Headers should avoid #include "dependency.h" where possible
  * do forward declaration of classes instead where possible
```C++
class ForwardDeclaredType;

void someFunction(ForwardDeclaredType &param);
```
  * 'not possible' includes templates, non-class types, superclasses, try building it without and see what fails
