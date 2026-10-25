#include <cstddef>
#include <cstdint>
#include <type_traits>


enum class OrderSide : std::uint8_t{	//1 bytes
	 buy,
	 sell
};

//This is to ensure the memory address always start with multiples of 32 for the assertion
//sizeof(Type) must be a multiple of alignof(Type)
//This is a memory layout and cache efficiency and it is to ensure cache will not over run 64 bytes per line 
//If over run 64 computer will have to fetch more for ram making it slower.
//For example if leave it alignas 8 bytes, it will work but if the memory starts ar 48 with 32 bytes of struct it will stretch to 79.
//Causing the comptuer to read the other half of after 64
// alignas(32) prevents a 32-byte update from straddling a 64-byte cache-line
// boundary, provided the object is stored using an alignment-aware mechanism.
struct alignas(32) L2OrderUpdate {	//32 bytes with 3 padding at the end
	double price;
	double quantity;
	std::uint64_t timestamp;	//To ensure it is 64 bits
	std::uint32_t orderCount;	//To ensure it is 32 bits
	OrderSide side;
	char bytes[3];
};

//Static assertion to ensure certain conditions are met before procedure
// sizeof: Confirms the total structure size, including padding, is exactly 32 bytes.
// alignof: Confirms each structure instance begins on a 32-byte memory boundary.
// is_trivial_v: Confirms the type has no complex construction or destruction behavior.
// is_standard_layout_v: Confirms the members follow predictable C-like layout rules.
// is_trivially_copyable_v: Confirms the object representation can be copied between
//                          compatible objects of the same type using raw byte copying.

static_assert(
	sizeof(L2OrderUpdate) == 32,
	"L2OrderUpdate must be exactly 32 bytes"
);

static_assert(
	alignof(L2OrderUpdate) == 32,
	"L2OrderUpdate must be aligned to 32 bytes"
);

static_assert(	//To ensure ti has no complex construction nor destruction behavior
	std::is_trivial_v<L2OrderUpdate>,
	"L2OrderUpdate must remain a trival type"
);

static_assert(	//Memeory layout to follow predictable C++ rules
	std::is_standard_layout_v<L2OrderUpdate>,
	"L2OrderUpdate must have aa standard memory layout"
);

static_assert(	//Alway can be copied as raw bytes safely
	std::is_trivially_copyable_v<L2OrderUpdate>,
	"L2OrderUpdate must be trivally copyable"
);



