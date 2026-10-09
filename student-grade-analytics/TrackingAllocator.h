#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>

/// Running totals of bytes requested through TrackingAllocator.
/// (Not thread-safe; intended for single-threaded benchmarks.)
struct AllocationCounters {
	std::size_t current = 0;     // bytes live right now
	std::size_t peak = 0;        // highest `current` since reset
	std::size_t total = 0;       // bytes ever requested
	std::size_t allocations = 0; // number of allocate() calls

	void reset() { *this = AllocationCounters{}; }
};

inline AllocationCounters& allocationCounters()
{
	static AllocationCounters counters;
	return counters;
}

/// std::allocator wrapper that records how much memory a container asks for.
/// It counts *requested* bytes only; the heap's own per-allocation overhead
/// (often 8-16 bytes) is not included, so node-based containers such as
/// std::list and std::map use somewhat more real memory than reported.
template <typename T>
struct TrackingAllocator {
	using value_type = T;

	TrackingAllocator() noexcept = default;

	template <typename U>
	TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

	T* allocate(std::size_t n)
	{
		AllocationCounters& c = allocationCounters();
		std::size_t bytes = n * sizeof(T);

		c.current += bytes;
		c.total += bytes;
		c.allocations++;
		c.peak = (std::max)(c.peak, c.current);

		return std::allocator<T>{}.allocate(n);
	}

	void deallocate(T* pointer, std::size_t n) noexcept
	{
		allocationCounters().current -= n * sizeof(T);
		std::allocator<T>{}.deallocate(pointer, n);
	}

	template <typename U>
	bool operator==(const TrackingAllocator<U>&) const noexcept { return true; }

	template <typename U>
	bool operator!=(const TrackingAllocator<U>&) const noexcept { return false; }
};
