#include <array>
#include <atomic>
#include <cstddef>

//Atomic: .relaxed makes the index access atomic without ordering other data
//		  .acquire pairs with a release operation to make earlier writes from the other thread visable
//		  .release publishes writes so a thread that acquire-loads the atomic can see them
//		  .load reads the value
//		  .store stores the value

/// <summary>
///  To ensure Single producer and single consumer. Producer threads calls push() to add items and consumer threads call pop() to remove them.
///	 Fixed array stores the items in between.
/// </summary>
/// <typeparam name="T"> Type of each element stored in the queue. </typeparam>
/// <typeparam name="Capacity"> Maximum number of elements the queue can hold. </typeparam>
template<typename T, std::size_t Capacity>
class SPSCQueue {
public:
	[[nodiscard]] //Ask complier to warn if caller ignores the return value
	bool push(const T& value) noexcept {
		// Read the index this thread owns; this atomic read is safe between threads.
		const std::size_t tail = m_tail.load(std::memory_order_relaxed);	//Reads the value

		// Check the consumer's index before reusing a slot.
		// Acquire pairs with the consumer's release-store after it reads an item.
		const std::size_t head = m_head.load(std::memory_order_acquire);

		//The difference between the counters is the number of queued items.
		//If it equals the capacity, every slot is occupied
		if (tail - head == Capacity)
			return false;

		const std::size_t slot = tail & (Capacity - 1);

		//Capacity is a power of two, so this mask wraps the increasing logical counter into a valid array index
		storage[slot] = value;

		//Release publishes both the item write and the updated queue position.
		//The consumer's acquire-load of m_tail will then see the item.
		m_tail.store(tail + 1, std::memory_order_release);	//Writes the value

		return true;
	}

	[[nodiscard]]
	bool pop(T& value) noexcept {

		// Read the index this thread owns.
		const std::size_t head = m_head.load(std::memory_order_relaxed);


		// Check whether the producer has published an item.
		// Acquire makes the producer's item write visible before we read the slot.
		const std::size_t tail = m_tail.load(std::memory_order_acquire);

		if (head == tail) { //Empty queue
			return false;
		}

		const std::size_t slot = head & (Capacity - 1);

		value = storage[slot];

		m_head.store(head + 1, std::memory_order_release);

		return true;
	}

private:
	static_assert(Capacity > 0);
	static_assert((Capacity& (Capacity - 1)) == 0,
		"Capacity must be a power of two");

	std::array<T, Capacity> storage{};

	//std::atomic lets threads safely ready and update one shared value without mutex. Helps with cahce-line interference.
	//Aligning them as 64 bytes allows lesser interference therefore the lesser the chances for data races.
	alignas(64) std::atomic<std::size_t> m_head{ 0 };	//Points to next item for consumer. After it reads then consumer advance m_head
	alignas(64) std::atomic<std::size_t> m_tail{ 0 };	//Points to next slot for producer. After it writes, producer advances m_tail
};
