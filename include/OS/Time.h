#ifndef OS_TIME_ABSTRACTION_H
#define OS_TIME_ABSTRACTION_H

#include <chrono>
#include <thread>

namespace CanMoveIt{

using Nanoseconds = std::chrono::duration<int64_t, std::nano>;
using Microseconds = std::chrono::duration<int64_t, std::micro> ;
using Milliseconds =std::chrono::duration<int64_t, std::milli> ;
using Seconds = std::chrono::duration<double>;

using TimePoint = std::chrono::high_resolution_clock::time_point;
using Duration  = std::chrono::high_resolution_clock::duration;

inline TimePoint GetTimeNow() { return std::chrono::high_resolution_clock::now(); }

template <class T, class D> T duration_cast (D tm)
{
   return std::chrono::duration_cast<T>( tm );
}


template <class T> T ElapsedTime( const TimePoint  prev_time, const TimePoint  next_time)
{
    return (std::chrono::duration_cast<T>(next_time - prev_time) );
}

inline void sleepUntil(TimePoint t) { std::this_thread::sleep_until( t ); }

inline void sleepFor(Duration d) { std::this_thread::sleep_for( d ); }

}

#endif // OS_TIME_ABSTRACTION_H
